#pragma once

#include <CD/Common/Debug.hpp>
#include <CD/Graphics/Model.hpp>
#include <CD/Graphics/Frame.hpp>
#include <CD/Graphics/Scene.hpp>
#include <vector>

namespace CD {

struct MeshInstance {
	const MaterialInstance* material;
	const Mesh* mesh;
	BufferAllocation instance_data;
};

enum RenderQueueConsumer : std::uint8_t {
	RenderQueueConsumer_DepthPass,
	RenderQueueConsumer_Geometry,
	RenderQueueConsumer_Count
};

class RenderQueue {
public:
	RenderQueue(RenderQueueConsumer);

	void setup(GPUBufferAllocator&, const void* queue_data, std::uint32_t num_bytes);
	void add_mesh(const MaterialInstance*, const Mesh&, const Matrix4x4&, std::uint32_t transform_index);
	void build();
	void reset();

	const BufferAllocation& get_buffer() const;
	const std::vector<MeshInstance>& get_meshes() const;
	const std::vector<std::uint32_t>& get_indices() const;

	const GPU::Signal& get_copy_fence() const;
private:
	RenderQueueConsumer type;

	GPUBufferAllocator* buffer_allocator;
	BufferAllocation queue_data_buffer;
	GPU::Signal copy_fence;

	std::vector<std::uint32_t> indices;
	std::vector<MeshInstance> meshes;
	std::vector<std::uint64_t> sort_keys;

	struct MeshInstanceBuffer {
		std::uint32_t transform_index;
		std::uint32_t material_index;
	};

	std::uint64_t get_sort_key(const Mesh&, float depth, const MaterialInstance*);
};

constexpr GPU::BufferFormat gbuffer_render_target_formats[] {
	GPU::BufferFormat::R32G32B32A32_UINT,
	GPU::BufferFormat::R16G16_SNORM,
	GPU::BufferFormat::R16G16B16A16_SNORM,
	GPU::BufferFormat::R16_UINT
};

class Renderer {
public:
	Renderer(Frame&);

	void add_model(const Model*, const Transform&);
	void build(const Camera&);
	void clear_render_state();

	void draw_gbuffer(GPU::CommandBuffer&);
	void draw_depth(GPU::CommandBuffer&);
private:
	Frame& frame;

	RenderQueue gbuffer_queue;
	RenderQueue depth_queue;

	std::vector<const Model*> frame_models;
	std::vector<Matrix4x4> frame_transforms;
	BufferAllocation transform_buffer;
	GPU::Signal upload_fence;

	const GraphicsPipeline* gbuffer_pipeline;
	const GraphicsPipeline* depth_pipeline;

	struct GBufferConstants {
		Matrix4x4 view_projection;
	};

	struct DepthPassConstants {
		Matrix4x4 view_projection;
	};

	enum RendererInputSlot : std::uint8_t {
		RendererInputSlot_Transforms,
		RendererInputSlot_RenderQueueConstants,
		RendererInputSlot_MeshInstance,
		RendererInputSlot_Count
	};

	void set_global_state(GPU::PipelineInputState&);
	void set_render_queue_state(GPU::PipelineInputState&, const RenderQueue&);
	void set_instance_state(GPU::PipelineInputState&, const MeshInstance&);

	void copy_frame_data();
};

}