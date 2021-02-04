#pragma once

#include <CD/Graphics/Common.hpp>
#include <CD/GPU/CommandBuffer.hpp>
#include <CD/GPU/Shader.hpp>
#include <vector>
#include <queue>

namespace CD {

struct BufferAllocation {
	GPU::BufferHandle handle;
	std::uint32_t offset;
};

class GPUBufferAllocator {
public:
	GPUBufferAllocator(GPU::Device&);
	~GPUBufferAllocator();

	BufferAllocation create_buffer(std::uint32_t buffer_size, const void* data = nullptr, bool upload = true);
	void lock(const GPU::Signal& frame_fence);
	void reset(GPU::Signal& completed_fence);

	void update_data();
	const GPU::Signal& flush();
	const GPU::Signal& get_copy_fence() const;
private:
	constexpr static std::uint64_t upload_buffer_size = 1 << 27;
	constexpr static std::uint32_t buffer_alignment = 256;

	GPU::Device& device;
	GPU::CommandBuffer command_buffer;

	GPU::BufferHandle gpu_buffer;
	GPU::BufferHandle upload_buffer;
	void* mapped_memory;

	struct BufferFrameData {
		std::uint32_t tail;
		std::uint32_t head;
	};

	BufferFrameData frame;
	GPU::Signal frame_fence;
	GPU::Signal copy_fence;

	std::vector<BufferFrameData> uploads;

	std::queue<std::pair<GPU::Signal, BufferFrameData>> cache;
};

class CopyContext {
public:
	CopyContext(GPU::Device&);
	~CopyContext();

	void upload_texture_slice(const Texture*, const GPU::TextureView&, const void* data, std::uint16_t width, std::uint16_t height, std::uint32_t row_pitch);
	void upload_buffer(const GPU::BufferView&, const void* data);

	void flush();
private:
	constexpr static std::uint64_t texture_slice_alignment = 512;
	constexpr static std::uint64_t texture_row_alignment = 256;
	constexpr static std::uint64_t upload_buffer_size = 1 << 27;

	GPU::Device& device;
	GPU::CommandBuffer command_buffer;

	GPU::BufferHandle upload_buffer_handle;
	void* mapped_buffer;
	std::uint32_t curr_offset;

	GPU::Signal copy_fence;
	GPU::Signal completed;

	struct CopyBufferAllocation {
		std::uint32_t offset;
		std::uint32_t size;
		GPU::Signal fence;
	};

	std::vector<CopyBufferAllocation> cache;
	std::vector<CopyBufferAllocation> free_list;

	CopyBufferAllocation reserve(std::uint64_t num_bytes, std::uint64_t alignment = texture_row_alignment);
};

using FrameResourceIndex = std::uint32_t;

struct FrameTextureViews {
	GPU::PipelineHandle srv;
	GPU::PipelineHandle uav;
};

struct FrameTexture {
	Texture texture;
	GPU::ResourceState state = GPU::ResourceState::Common;
	FrameTextureViews* views;
};

class Frame {
public:
	Frame(GPU::Device&, float width, float height);
	~Frame();

	FrameResourceIndex add_texture(GPU::TextureDesc&, bool create_views_flag = true);
	FrameTexture& get_texture(FrameResourceIndex);

	const RenderPass* create_render_pass(const GPU::RenderPassDesc&);
	const GraphicsPipeline* create_pipeline(const GPU::GraphicsPipelineDesc&, const GPU::PipelineInputLayout&);
	const ComputePipeline* create_pipeline(const GPU::ComputePipelineDesc&, const GPU::PipelineInputLayout&);

	void bind_resources(const FrameResourceIndex* textures, std::size_t num_textures, GPU::ResourceState);

	void begin();
	void present();
	void wait();
	bool resize_buffers(float width, float height);

	const GPU::Viewport& get_viewport() const;
	GPU::ShaderCompiler& get_shader_compiler();
	GPU::Device& get_device();
	GPU::CommandBuffer& get_command_buffer();
	GPUBufferAllocator& get_buffer_allocator();
	CopyContext& get_copy_context();
private:
	static constexpr std::uint32_t max_latency = 3;

	GPU::ShaderCompiler shader_compiler;

	GPU::Device& device;
	GPU::CommandBuffer command_buffer;
	GPU::Viewport viewport;

	GPU::Signal present_fences[max_latency];
	GPU::Signal completed_fence;
	std::uint32_t present_index;

	GPUBufferAllocator buffer_allocator;
	CopyContext copy_context;

	std::vector<std::unique_ptr<FrameTexture>> texture_pool;
	std::vector<std::unique_ptr<FrameTextureViews>> views;

	std::vector<std::unique_ptr<RenderPass>> render_passes;
	std::vector<std::unique_ptr<ComputePipeline>> compute_pipelines;
	std::vector<std::unique_ptr<GraphicsPipeline>> graphics_pipelines;

	void create_views(FrameTexture&);
	void destroy_textures();
};

}