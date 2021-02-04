#pragma once

#include <CD/GPU/D3D12/Common.hpp>
#include <CD/GPU/CommandBuffer.hpp>
#include <vector>

namespace CD::GPU::D3D12 {

constexpr D3D12_RESOURCE_BARRIER get_resource_transition(ID3D12Resource* resource, std::uint32_t subresource, D3D12_RESOURCE_STATES state_before, D3D12_RESOURCE_STATES state_after) {
	return {
		D3D12_RESOURCE_BARRIER_TYPE_TRANSITION,
		D3D12_RESOURCE_BARRIER_FLAG_NONE,
		{resource, subresource, state_before, state_after}
	};
}

constexpr std::uint32_t texture_subresource(std::uint32_t mip, std::uint32_t index, std::uint32_t plane, std::uint32_t mip_count, std::uint32_t array_size) {
	return mip + mip_count * index + mip_count * array_size * plane;
}

constexpr D3D12_COMMAND_LIST_TYPE d3d12_command_list_type(CommandQueueType type) {
	CD_ASSERT(type < CommandQueueType_Count);

	switch(type) {
	case CommandQueueType_Direct:
		return D3D12_COMMAND_LIST_TYPE_DIRECT;
	case CommandQueueType_Compute:
		return D3D12_COMMAND_LIST_TYPE_COMPUTE;
	case CommandQueueType_Copy:
		return D3D12_COMMAND_LIST_TYPE_COPY;
	}

	return D3D12_COMMAND_LIST_TYPE_BUNDLE;
}

struct Fence {
	ID3D12Fence* fence;
	std::uint64_t head;
	std::uint64_t tail;
	std::uint64_t last_signal;
};

enum class CommandListState {
	Recording,
	Pending,
	Reset
};

class CommandList {
	using CommandAllocator = std::pair<ID3D12CommandAllocator*, std::uint64_t>;
public:
	CommandList(const Adapter&, const Fence&, DeviceResources&, D3D12_COMMAND_LIST_TYPE, const ShaderDescriptorHeap*);
	~CommandList();

	CommandListState get_state();
	void lock();
	void close();
	void reset();

	ID3D12CommandList* d3d12_command_list();

	void transition_barrier(const void*);
	void uav_barrier(const void*);
	void begin_render_pass(const void*);
	void end_render_pass();
	void copy_buffer(const void*);
	void copy_texture(const void*);
	void copy_buffer_to_texture(const void*);
	void copy_texture_to_buffer(const void*);
	void dispatch(const void*);
	void dispatch_indirect(const void*, ID3D12CommandSignature*);
	void draw(const void*);
	void copy_to_swapchain(const void*, const SwapChain&);
	void insert_timestamp(const void*, ID3D12QueryHeap*);
	void resolve_timestamps(const void*, ID3D12QueryHeap*);
private:
	const Adapter& adapter;
	const Fence& fence;
	D3D12_COMMAND_LIST_TYPE type;
	CommandListState state;

	DeviceResources& resources;
	const ShaderDescriptorHeap* descriptor_heap;
	DescriptorPool rtv_pool;
	DescriptorPool dsv_pool;

	ID3D12GraphicsCommandList5* command_list;
	std::vector<CommandAllocator> allocators;
	CommandAllocator* current_allocator;

	enum class BindPoint {
		None,
		Compute,
		Graphics
	};

	struct BarrierBuffer {
		D3D12_RESOURCE_BARRIER barriers[10];
		std::uint32_t barrier_count;
	};

	struct RootSignatureState {
		PipelineInputState arguments;
		bool cleared;
	};

	BindPoint bind_point;
	BarrierBuffer barrier_buffer;
	PipelineState graphics_pso;
	PipelineState compute_pso;
	RootSignatureState graphics_arguments;
	RootSignatureState compute_arguments;
	RenderPass* current_render_pass;

	void set_descriptor_heap(const ShaderDescriptorHeap&);
	void issue_barriers();
	void set_compute_state(const PipelineState&, const PipelineInputState&);
	void set_graphics_state(const PipelineState&, const PipelineInputState&);
	void add_transition(ID3D12Resource*, std::uint32_t index, D3D12_RESOURCE_STATES before, D3D12_RESOURCE_STATES after);
};

class Engine {
public:
	Engine(const Adapter&, DeviceResources&, const ShaderDescriptorHeap&);
	~Engine();

	void set_swapchain(const SwapChain*);
	CommandList& get_command_list(CommandQueueType);

	ID3D12CommandQueue* get_queue(CommandQueueType) const;

	void block(const Signal&);
	void signal_queue(CommandQueueType);
	void wait(const Signal&, CommandQueueType);

	void flush_queue(CommandQueueType);
	void flush();
	void sync();

	Signal submit_command_buffer(const CommandBuffer&, CommandQueueType);
	Signal present();
private:
	struct CommandQueue {
		ID3D12CommandQueue* queue;
		Fence fence;
		std::vector<ID3D12CommandList*> command_list_buffer;
	};

	const Adapter& adapter;
	DeviceResources& resources;
	const ShaderDescriptorHeap& descriptor_heap;
	const SwapChain* swapchain;

	CommandQueue queues[CommandQueueType_Count];
	std::vector<std::unique_ptr<CommandList>> command_list_pool[CommandQueueType_Count];

	ID3D12QueryHeap* timing_heap;
	ID3D12CommandSignature* dispatch_indirect_signature;
};

inline CommandList& Engine::get_command_list(CommandQueueType type) {
	CommandQueue& queue = queues[type];
	for(auto& command_list : command_list_pool[type]) {
		if(command_list->get_state() == CommandListState::Reset) {
			command_list->lock();
			return *command_list;
		}
	}

	const ShaderDescriptorHeap* dh = type != CommandQueueType_Copy ? &descriptor_heap : nullptr;
	return *command_list_pool[type].emplace_back(std::make_unique<CommandList>(adapter, queues[type].fence, resources, d3d12_command_list_type(type), dh));
}

}