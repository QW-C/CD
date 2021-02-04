#pragma once

#include <CD/GPU/D3D12/Common.hpp>
#include <CD/GPU/D3D12/Allocator.hpp>
#include <CD/GPU/D3D12/Engine.hpp>
#include <CD/GPU/Device.hpp>
#include <memory>

namespace CD::GPU::D3D12 {

class Device : public GPU::Device {
public:
	Device(Adapter&, const SwapChainDesc*, IDXGIFactory7*);
	~Device();

	BufferHandle create_buffer(const BufferDesc&) final;
	TextureHandle create_texture(const TextureDesc&) final;
	PipelineHandle create_render_pass(const RenderPassDesc&);
	PipelineHandle create_pipeline_input_list(std::uint32_t num_descriptors) final;
	PipelineHandle create_pipeline_state(const GraphicsPipelineDesc&, const PipelineInputLayout&) final;
	PipelineHandle create_pipeline_state(const ComputePipelineDesc&, const PipelineInputLayout&) final;

	void destroy_buffer(BufferHandle) final;
	void destroy_texture(TextureHandle) final;
	void destroy_pipeline_resource(PipelineHandle) final;

	void map_buffer(BufferHandle, void** data, std::uint64_t offset, std::uint64_t size) final;
	void unmap_buffer(BufferHandle, std::uint64_t offset, std::uint64_t size) final;

	void update_pipeline_input_list(PipelineHandle, DescriptorType, const TextureView* views, std::uint64_t num_textures, std::uint64_t offset) final;
	void update_pipeline_input_list(PipelineHandle, DescriptorType, const BufferView* views, std::uint64_t num_buffers, std::uint64_t offset) final;

	void signal(CommandQueueType) final;
	void wait(const Signal& producer, CommandQueueType queue) final;
	void wait_for_fence(const Signal&) final;
	Signal submit_commands(const CommandBuffer&, CommandQueueType) final;
	Signal reset() final;

	void resize_buffers(std::uint32_t width, std::uint32_t height) final;
	DeviceFeatureInfo report_feature_info() final;
private:
	Adapter& adapter;
	Allocator allocator;
	DeviceResources resources;
	ShaderDescriptorHeap shader_descriptor_heap;
	Engine engine;

	DescriptorPool rtv_pool;
	std::unique_ptr<SwapChain> swapchain;

	ID3D12RootSignature* create_root_signature(const PipelineInputLayout&, bool ia = false);
};

}