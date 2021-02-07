#pragma once

#include <CD/Graphics/Renderer.hpp>
#include <algorithm>
#include <DirectXCollision.h>

namespace CD {

RenderQueue::RenderQueue(RenderQueueConsumer type) :
	type(type),
	buffer_allocator(nullptr),
	queue_data_buffer(),
	copy_fence() {
}

void RenderQueue::setup(GPUBufferAllocator& allocator, const void* queue_data, std::uint32_t num_bytes) {
	buffer_allocator = &allocator;
	queue_data_buffer = buffer_allocator->create_buffer(num_bytes, queue_data);
}

void RenderQueue::add_mesh(const MaterialInstance* material, const Mesh& mesh, const Matrix4x4&, std::uint32_t transform_index) {
	MeshInstance instance;
	instance.mesh = &mesh;
	instance.material = material;

	std::uint32_t material_index = material ? material->get_constants().index : 0;
	MeshInstanceBuffer instance_constants {transform_index, material_index};
	instance.instance_data = buffer_allocator->create_buffer(sizeof(MeshInstanceBuffer), &instance_constants);

	meshes.push_back(instance);
	sort_keys.push_back(0);
	indices.push_back(static_cast<std::uint32_t>(indices.size()));
}

void RenderQueue::build() {
	buffer_allocator->update_data();
	copy_fence = buffer_allocator->flush();

	if(type == RenderQueueConsumer_DepthPass) {
		std::stable_sort(indices.begin(), indices.end(), [this](std::uint32_t l, std::uint32_t r) { return sort_keys[l] < sort_keys[r]; });
	}
}

void RenderQueue::reset() {
	indices.clear();
	meshes.clear();
	sort_keys.clear();
}

const BufferAllocation& RenderQueue::get_buffer() const {
	return queue_data_buffer;
}

const std::vector<MeshInstance>& RenderQueue::get_meshes() const {
	return meshes;
};

const std::vector<std::uint32_t>& RenderQueue::get_indices() const {
	return indices;
}

const GPU::Signal& RenderQueue::get_copy_fence() const {
	return copy_fence;
}

std::uint64_t RenderQueue::get_sort_key(const Mesh& mesh, float depth, const MaterialInstance* material) {
	switch(type) {
	case RenderQueueConsumer_DepthPass: {
		return reinterpret_cast<std::uint32_t&>(depth);
	}
	case RenderQueueConsumer_Geometry: {
		/*std::uint32_t hi = material->get_hash();
		std::uint32_t lo = static_cast<std::uint32_t>(mesh.input_buffer.input_buffer.handle);
		return static_cast<std::uint64_t>(hi) << 32 | lo;*/
		return 0;
	}
	}

	return 0;
}

static void set_input_buffer(GPU::DrawDesc& desc, const IndexedInputBuffer& buffer) {
	desc.input_buffer = buffer.input_buffer;
	desc.index_buffer = buffer.index_buffer;
	desc.index_buffer_size = buffer.index_buffer_size;
	desc.num_instances = 1;
	desc.num_elements = buffer.num_indices;
	desc.num_vertex_buffers = MeshVertexElement_Count;
	for(std::size_t element_index = 0; element_index < MeshVertexElement_Count; ++element_index) {
		desc.vertex_buffer_offsets[element_index] = buffer.vertex_buffer_offsets[element_index];
		desc.vertex_buffer_strides[element_index] = buffer.vertex_buffer_strides[element_index];
		desc.vertex_buffer_sizes[element_index] = buffer.vertex_buffer_sizes[element_index];
	}
}

static bool frustum_aabb_intersection(const AABB& aabb, const Matrix4x4& transform, const Camera& camera) {
	using namespace DirectX;

	XMVECTOR min = XMLoadFloat3(&aabb.min);
	XMVECTOR max = XMLoadFloat3(&aabb.max);
	XMVECTOR center = (min + max) / 2.f;
	XMVECTOR extents = (max - min) / 2.f;

	Vector3 vextents, vcenter;
	XMStoreFloat3(&vextents, extents);
	XMStoreFloat3(&vcenter, center);

	BoundingBox b(vcenter, vextents);
	XMMATRIX t = XMLoadFloat4x4(&transform);
	b.Transform(b, t);

	XMMATRIX p = XMLoadFloat4x4(&camera.get_projection());
	BoundingFrustum frustum(p);
	XMMATRIX frustum_transform = XMLoadFloat4x4(&camera.get_transform());
	frustum.Transform(frustum, frustum_transform);

	return frustum.Intersects(b);
}

Renderer::Renderer(Frame& frame) :
	frame(frame),
	gbuffer_queue(RenderQueueConsumer_Geometry),
	depth_queue(RenderQueueConsumer_DepthPass),
	transform_buffer(),
	upload_fence() {

	GPU::PipelineInputLayout layout {};
	layout.num_entries = RendererInputSlot_Count;
	layout.entries[RendererInputSlot_Transforms] = GPU::pipeline_input_buffer_defaults(GPU::DescriptorType::SRV, 0, 0);
	layout.entries[RendererInputSlot_RenderQueueConstants] = GPU::pipeline_input_buffer_defaults(GPU::DescriptorType::CBV, 0, 0);
	layout.entries[RendererInputSlot_MeshInstance] = GPU::pipeline_input_buffer_defaults(GPU::DescriptorType::CBV, 1, 0);

	GPU::ShaderCompiler& compiler = frame.get_shader_compiler();

	{
		GPU::CompileShaderDesc vs_desc {
			L"Resources/Shaders/DepthPass.hlsl",
			L"vs_main",
			GPU::ShaderStage::Vertex
		};

		GPU::ShaderPtr depth_vs = compiler.compile_shader(vs_desc);

		GPU::GraphicsPipelineDesc depth_pipeline_desc = GPU::graphics_pipeline_defaults(0);
		depth_pipeline_desc.vs = {depth_vs->GetBufferPointer(), depth_vs->GetBufferSize()};

		GPU::InputElement vertex_elements[] {
			GPU::input_element_defaults("POSITION", GPU::BufferFormat::R32G32B32_FLOAT, 0, 0),
		};
		depth_pipeline_desc.input_layout = {vertex_elements, 1};

		depth_pipeline_desc.depth_stencil_format = GPU::BufferFormat::D32_FLOAT_S8X24_UINT;
		depth_pipeline_desc.depth_stencil = GPU::depth_stencil_defaults(true);
		depth_pipeline_desc.depth_stencil.depth_write_mask = GPU::DepthWriteMask::All;

		depth_pipeline = frame.create_pipeline(depth_pipeline_desc, layout);
	}

	{
		GPU::CompileShaderDesc vs_desc {
			L"Resources/Shaders/GBuffer.hlsl",
			L"vs_main",
			GPU::ShaderStage::Vertex
		};

		GPU::CompileShaderDesc ps_desc {
			L"Resources/Shaders/GBuffer.hlsl",
			L"ps_main",
			GPU::ShaderStage::Pixel
		};

		GPU::ShaderPtr gbuffer_vs = compiler.compile_shader(vs_desc);
		GPU::ShaderPtr gbuffer_ps = compiler.compile_shader(ps_desc);

		GPU::GraphicsPipelineDesc gbuffer_desc = GPU::graphics_pipeline_defaults(static_cast<std::uint32_t>(std::size(gbuffer_render_target_formats)));
		gbuffer_desc.vs = {gbuffer_vs->GetBufferPointer(), gbuffer_vs->GetBufferSize()};
		gbuffer_desc.ps = {gbuffer_ps->GetBufferPointer(), gbuffer_ps->GetBufferSize()};

		GPU::InputElement vertex_elements[] {
			GPU::input_element_defaults("POSITION", GPU::BufferFormat::R32G32B32_FLOAT, 0, 0),
			GPU::input_element_defaults("NORMAL", GPU::BufferFormat::R32G32B32_FLOAT, 1, 0),
			GPU::input_element_defaults("TANGENT", GPU::BufferFormat::R32G32B32_FLOAT, 2, 0),
			GPU::input_element_defaults("UV", GPU::BufferFormat::R32G32_FLOAT, 3, 0),
		};
		gbuffer_desc.input_layout = {vertex_elements, static_cast<std::uint32_t>(std::size(vertex_elements))};

		gbuffer_desc.depth_stencil_format = GPU::BufferFormat::D32_FLOAT_S8X24_UINT;
		gbuffer_desc.depth_stencil = GPU::depth_stencil_defaults(true);
		gbuffer_desc.depth_stencil.depth_write_mask = GPU::DepthWriteMask::Zero;

		for(std::size_t i = 0; i < std::size(gbuffer_render_target_formats); ++i) {
			gbuffer_desc.render_target_format[i] = gbuffer_render_target_formats[i];
		}

		gbuffer_pipeline = frame.create_pipeline(gbuffer_desc, layout);
	}
}

void Renderer::add_model(const Model* model, const Transform& transform) {
	frame_models.push_back(model);

	frame_transforms.push_back(matrix_transform(transform));
}

void Renderer::build(const Camera& camera) {
	copy_frame_data();

	GBufferConstants gbuffer_constants {
		camera.get_view_projection()
	};
	gbuffer_queue.setup(frame.get_buffer_allocator(), &gbuffer_constants, sizeof(GBufferConstants));

	DepthPassConstants depth_constants {
		camera.get_view_projection()
	};
	depth_queue.setup(frame.get_buffer_allocator(), &depth_constants, sizeof(DepthPassConstants));
	
	for(std::size_t model_index = 0; model_index < frame_models.size(); ++model_index) {
		const auto& meshes = frame_models[model_index]->get_meshes();
		const auto& materials = frame_models[model_index]->get_materials();

		const Matrix4x4& transform = frame_transforms[model_index];

		/*DirectX::XMMATRIX t = DirectX::XMLoadFloat4x4(&transform) * DirectX::XMLoadFloat4x4(&camera.get_view());
		Matrix4x4 transform_vs;
		DirectX::XMStoreFloat4x4(&transform_vs, t);*/

		for(std::size_t mesh_index = 0; mesh_index < meshes.size(); ++mesh_index) {
			const Mesh& mesh = *meshes[mesh_index];

			if(frustum_aabb_intersection(mesh.get_aabb(), transform, camera)) {
				gbuffer_queue.add_mesh(materials[mesh_index], mesh, transform, static_cast<std::uint32_t>(model_index));
				depth_queue.add_mesh(nullptr, mesh, transform, static_cast<std::uint32_t>(model_index));
			}
		}
	}

	depth_queue.build();
	gbuffer_queue.build();
}

void Renderer::clear_render_state() {
	depth_queue.reset();
	gbuffer_queue.reset();

	frame_models.clear();
	frame_transforms.clear();
}

void Renderer::draw_gbuffer(GPU::CommandBuffer& command_buffer) {
	GPU::PipelineInputState input_state {};
	input_state.num_elements = RendererInputSlot_Count;

	set_global_state(input_state);
	set_render_queue_state(input_state, gbuffer_queue);

	const GPU::Viewport& vp = frame.get_viewport();

	for(const MeshInstance& instance : gbuffer_queue.get_meshes()) {
		const IndexedInputBuffer& input_buffer = instance.mesh->get_input_buffer();

		set_instance_state(input_state, instance);

		GPU::DrawDesc draw_desc {};
		draw_desc.graphics_pipeline = gbuffer_pipeline->handle;
		draw_desc.pipeline_input_state = input_state;

		draw_desc.rect = {
			0,
			0,
			static_cast<std::uint32_t>(vp.width),
			static_cast<std::uint32_t>(vp.height)
		};

		set_input_buffer(draw_desc, input_buffer);

		command_buffer.add_command(draw_desc);
	}
}

void Renderer::draw_depth(GPU::CommandBuffer& command_buffer) {
	GPU::PipelineInputState input_state {};
	input_state.num_elements = RendererInputSlot_Count;

	set_global_state(input_state);
	set_render_queue_state(input_state, depth_queue);

	const GPU::Viewport& vp = frame.get_viewport();

	const auto& indices = depth_queue.get_indices();
	const auto& depth_queue_meshes = depth_queue.get_meshes();
	for(std::size_t index = 0; index < indices.size(); ++index) {
		const MeshInstance& instance = depth_queue_meshes[index];

		const IndexedInputBuffer& input_buffer = instance.mesh->get_input_buffer();

		set_instance_state(input_state, instance);

		GPU::DrawDesc draw_desc {};
		draw_desc.graphics_pipeline = depth_pipeline->handle;
		draw_desc.pipeline_input_state = input_state;

		draw_desc.rect = {
			0,
			0,
			static_cast<std::uint32_t>(vp.width),
			static_cast<std::uint32_t>(vp.height)
		};

		draw_desc.input_buffer = input_buffer.input_buffer;
		draw_desc.index_buffer = input_buffer.index_buffer;
		draw_desc.index_buffer_size = input_buffer.index_buffer_size;
		draw_desc.num_elements = input_buffer.num_indices;
		draw_desc.num_instances = 1;
		draw_desc.num_vertex_buffers = 1;
		draw_desc.vertex_buffer_sizes[0] = input_buffer.vertex_buffer_sizes[0];
		draw_desc.vertex_buffer_strides[0] = sizeof(Vector3);

		command_buffer.add_command(draw_desc);
	}
}

void Renderer::set_global_state(GPU::PipelineInputState& state) {
	state.types[RendererInputSlot_Transforms] = GPU::PipelineInputGroupType::Buffer;
	state.input_elements[RendererInputSlot_Transforms].buffer.buffer = transform_buffer.handle;
	state.input_elements[RendererInputSlot_Transforms].buffer.offset = transform_buffer.offset;
	state.input_elements[RendererInputSlot_Transforms].buffer.type = GPU::DescriptorType::SRV;
}

void Renderer::set_render_queue_state(GPU::PipelineInputState& state, const RenderQueue& render_queue) {
	const BufferAllocation& buffer = render_queue.get_buffer();

	state.types[RendererInputSlot_RenderQueueConstants] = GPU::PipelineInputGroupType::Buffer;
	state.input_elements[RendererInputSlot_RenderQueueConstants].buffer.buffer = buffer.handle;
	state.input_elements[RendererInputSlot_RenderQueueConstants].buffer.offset = buffer.offset;
	state.input_elements[RendererInputSlot_RenderQueueConstants].buffer.type = GPU::DescriptorType::CBV;
}

void Renderer::set_instance_state(GPU::PipelineInputState& state, const MeshInstance& mesh) {
	state.types[RendererInputSlot_MeshInstance] = GPU::PipelineInputGroupType::Buffer;
	state.input_elements[RendererInputSlot_MeshInstance].buffer.buffer = mesh.instance_data.handle;
	state.input_elements[RendererInputSlot_MeshInstance].buffer.offset = mesh.instance_data.offset;
	state.input_elements[RendererInputSlot_MeshInstance].buffer.type = GPU::DescriptorType::CBV;
}

void Renderer::copy_frame_data() {
	GPUBufferAllocator& buffer_allocator = frame.get_buffer_allocator();
	transform_buffer = buffer_allocator.create_buffer(static_cast<std::uint32_t>(frame_transforms.size() * sizeof(Matrix4x4)), frame_transforms.data());

	upload_fence = buffer_allocator.flush();
}

}