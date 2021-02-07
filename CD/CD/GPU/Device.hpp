#pragma once

#include <CD/GPU/Common.hpp>

namespace CD::GPU {

class CommandBuffer;

class Device {
public:
	Device() = default;
	Device(const Device&) = delete;
	Device(Device&&) = delete;
	Device& operator=(const Device&) = delete;
	Device& operator=(Device&&) = delete;
	virtual ~Device() = default;

	virtual BufferHandle create_buffer(const BufferDesc&) = 0;
	virtual TextureHandle create_texture(const TextureDesc&) = 0;
	virtual PipelineHandle create_render_pass(const RenderPassDesc&) = 0;
	virtual PipelineHandle create_pipeline_input_list(std::uint32_t num_descriptors) = 0;
	virtual PipelineHandle create_pipeline_state(const GraphicsPipelineDesc&, const PipelineInputLayout&) = 0;
	virtual PipelineHandle create_pipeline_state(const ComputePipelineDesc&, const PipelineInputLayout&) = 0;

	virtual void destroy_buffer(BufferHandle) = 0;
	virtual void destroy_texture(TextureHandle) = 0;
	virtual void destroy_pipeline_resource(PipelineHandle) = 0;

	virtual void update_pipeline_input_list(PipelineHandle, DescriptorType, const TextureView* views, std::uint64_t num_textures, std::uint64_t offset) = 0;
	virtual void update_pipeline_input_list(PipelineHandle, DescriptorType, const BufferView* views, std::uint64_t num_buffers, std::uint64_t offset) = 0;

	virtual void map_buffer(BufferHandle, void** data, std::uint64_t offset, std::uint64_t size) = 0;
	virtual void unmap_buffer(BufferHandle, std::uint64_t offset, std::uint64_t size) = 0;

	virtual void signal(CommandQueueType) = 0;
	virtual void wait(const Signal& producer, CommandQueueType queue) = 0;
	virtual void wait_for_fence(const Signal& producer) = 0;
	virtual Signal submit_commands(const CommandBuffer&, CommandQueueType) = 0;
	virtual Signal reset() = 0;

	virtual void resize_buffers(std::uint32_t width, std::uint32_t height) = 0;
	virtual DeviceFeatureInfo report_feature_info() = 0;
};

}