#include <CD/Graphics/Sky.hpp>
#include <CD/Graphics/Frame.hpp>
#include <CD/GPU/Shader.hpp>

namespace CD {

Sky::Sky(Frame& frame) :
	frame(frame),
	texture(nullptr) {

	GPU::CompileShaderDesc vs_desc {
		L"Resources/Shaders/Sky.hlsl",
		L"vs_main",
		GPU::ShaderStage::Vertex
	};

	GPU::CompileShaderDesc ps_desc {
		L"Resources/Shaders/Sky.hlsl",
		L"ps_main",
		GPU::ShaderStage::Pixel
	};

	auto& compiler = frame.get_shader_compiler();
	auto vs = compiler.compile_shader(vs_desc);
	auto ps = compiler.compile_shader(ps_desc);

	GPU::GraphicsPipelineDesc pipeline_desc = GPU::graphics_pipeline_defaults(1, GPU::depth_stencil_defaults(true));

	pipeline_desc.vs = {vs->GetBufferPointer(), vs->GetBufferSize()};
	pipeline_desc.ps = {ps->GetBufferPointer(), ps->GetBufferSize()};

	pipeline_desc.depth_stencil.depth_write_mask = GPU::DepthWriteMask::Zero;
	pipeline_desc.depth_stencil_format = GPU::BufferFormat::D32_FLOAT_S8X24_UINT;

	pipeline_desc.render_target_format[0] = GPU::BufferFormat::R8G8B8A8_UNORM;

	GPU::PipelineInputLayout layout {};
	layout.num_entries = 2;
	layout.entries[0] = GPU::pipeline_input_buffer_defaults(GPU::DescriptorType::CBV, 0, 0);
	layout.entries[1] = GPU::pipeline_input_list_defaults(GPU::resource_list_defaults(1, GPU::DescriptorType::SRV, 0, 0));

	set_default_samplers(layout);

	graphics_pipeline = frame.create_pipeline(pipeline_desc, layout);

	skybox = frame.get_device().create_pipeline_input_list(1);
}

Sky::~Sky() = default;

void Sky::set_texture(const Texture* sky_texture) {
	texture = sky_texture;

	GPU::TextureView view = GPU::texture_view_defaults(texture->handle, texture->desc, 0, 0, 0, GPU::TextureViewDimension::TextureCube);

	frame.get_device().update_pipeline_input_list(skybox, GPU::DescriptorType::SRV, &view, 1, 0);
}

void Sky::render(GPU::CommandBuffer& cb, const Camera& camera) {
	if(texture) {
		GPU::PipelineInputState input {};
		input.num_elements = 2;

		SkyCameraConstants constants = {
			camera.get_transform(),
			camera.get_inv_projection()
		};
		BufferAllocation constant_buffer = frame.get_buffer_allocator().create_buffer(sizeof(SkyCameraConstants), &constants, false);

		input.types[0] = GPU::PipelineInputGroupType::Buffer;
		input.input_elements[0].buffer = {constant_buffer.handle, constant_buffer.offset, GPU::DescriptorType::CBV};

		input.types[1] = GPU::PipelineInputGroupType::ResourceList;
		input.input_elements[1].resource_list = skybox;

		GPU::DrawDesc draw {};

		const GPU::Viewport& vp = frame.get_viewport();
		draw.rect = {
			0,
			0,
			static_cast<std::uint32_t>(vp.width),
			static_cast<std::uint32_t>(vp.height)
		};

		draw.graphics_pipeline = graphics_pipeline->handle;
		draw.pipeline_input_state = input;

		draw.num_instances = 1;
		draw.num_elements = 3;

		cb.add_command(draw);
	}
}

}