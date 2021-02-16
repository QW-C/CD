#include <CD/GPU/D3D12/Engine.hpp>

namespace CD::GPU::D3D12 {

constexpr bool operator==(const PipelineInputBuffer& l, const PipelineInputBuffer& r) {
	return l.buffer == r.buffer &&
		l.offset == r.offset &&
		l.type == r.type;
}

constexpr bool operator==(const PipelineHandle& l, const PipelineHandle& r) {
	return l.handle == r.handle &&
		l.type == r.type;
}

CommandList::CommandList(const Adapter& adapter, const Fence& fence, DeviceResources& resources, D3D12_COMMAND_LIST_TYPE type, const ShaderDescriptorHeap* descriptor_heap) :
	adapter(adapter),
	fence(fence),
	type(type),
	state(CommandListState::Recording),
	resources(resources),
	descriptor_heap(descriptor_heap),
	rtv_pool(adapter, cpu_descriptor_count, D3D12_DESCRIPTOR_HEAP_TYPE_RTV),
	dsv_pool(adapter, cpu_descriptor_count, D3D12_DESCRIPTOR_HEAP_TYPE_DSV),
	command_list(nullptr),
	current_allocator(nullptr),
	barrier_buffer(),
	graphics_pso(),
	compute_pso(),
	graphics_arguments(),
	compute_arguments(),
	current_render_pass(nullptr) {

	ID3D12CommandAllocator* allocator = nullptr;
	HR_ASSERT(adapter.device->CreateCommandAllocator(type, IID_PPV_ARGS(&allocator)));
	CommandAllocator command_allocator {allocator, fence.tail};
	current_allocator = &allocators.emplace_back(command_allocator);

	ID3D12GraphicsCommandList* cl = nullptr;
	adapter.device->CreateCommandList(1 << adapter.node_index, type, allocator, nullptr, IID_PPV_ARGS(&cl));
	HR_ASSERT(cl->QueryInterface<ID3D12GraphicsCommandList5>(&command_list));
	cl->Release();

	if(descriptor_heap) {
		set_descriptor_heap(*descriptor_heap);
	}
}

CommandList::~CommandList() {
	for(const auto& [allocator, _] : allocators) {
		allocator->Release();
	}
	command_list->Release();
}

CommandListState CommandList::get_state() {
	return state;
}

void CommandList::lock() {
	CD_ASSERT(state == CommandListState::Reset);
	state = CommandListState::Recording;
}

void CommandList::close() {
	CD_ASSERT(state == CommandListState::Recording);

	issue_barriers();

	HR_ASSERT(command_list->Close());
	current_allocator->second = fence.head;

	state = CommandListState::Pending;
}

void CommandList::reset() {
	CD_ASSERT(state == CommandListState::Pending);

	ID3D12CommandAllocator* found_allocator = nullptr;

	for(CommandAllocator& allocator : allocators) {
		const auto& [allocator_ptr, allocator_fence] = allocator;

		if(allocator_fence <= fence.tail) {
			HR_ASSERT(allocator_ptr->Reset());
			found_allocator = allocator_ptr;

			current_allocator = &allocator;
			break;
		}
	}

	if(!found_allocator) {
		HR_ASSERT(adapter.device->CreateCommandAllocator(type, IID_PPV_ARGS(&found_allocator)));
		current_allocator = &allocators.emplace_back();
		current_allocator->first = found_allocator;
		current_allocator->second = ~0ui64;
	}

	HR_ASSERT(command_list->Reset(found_allocator, nullptr));


	if(descriptor_heap) {
		set_descriptor_heap(*descriptor_heap);
	}

	graphics_pso = {nullptr, nullptr};
	graphics_arguments.cleared = true;

	compute_pso = {nullptr, nullptr};
	compute_arguments.cleared = true;

	CD_ASSERT(!current_render_pass);

	state = CommandListState::Reset;
}

ID3D12CommandList* CommandList::d3d12_command_list() {
	return command_list;
}

void CommandList::set_descriptor_heap(const ShaderDescriptorHeap& shader_heap) {
	CD_ASSERT(type == D3D12_COMMAND_LIST_TYPE_DIRECT || type == D3D12_COMMAND_LIST_TYPE_COMPUTE);

	ID3D12DescriptorHeap* dh = descriptor_heap->get_descriptor_heap();
	command_list->SetDescriptorHeaps(1, &dh);
}

void CommandList::transition_barrier(const void* command_data) {
	const LayoutBarrierDesc& desc = *static_cast<const LayoutBarrierDesc*>(command_data);
	const TextureView& view = desc.texture;
	const Texture& texture = resources.texture_pool.get(view.texture);

	std::uint32_t subresource = desc.all_subresources ? D3D12_RESOURCE_BARRIER_ALL_SUBRESOURCES : texture_subresource(view.mip_level, view.index, view.plane, view.mip_count, view.depth);

	add_transition(texture.resource, subresource, d3d12_resource_state(desc.before), d3d12_resource_state(desc.after));
}

void CommandList::uav_barrier(const void* command_data) {
	const ResourceBarrierDesc& desc = *static_cast<const ResourceBarrierDesc*>(command_data);

	D3D12_RESOURCE_BARRIER barriers[max_resource_barriers];

	for(std::size_t i = 0; i < desc.num_buffer_barriers; ++i) {
		const Buffer& buffer = resources.buffer_pool.get(desc.buffers[i]);
		barriers[i].Type = D3D12_RESOURCE_BARRIER_TYPE_UAV;
		barriers[i].UAV.pResource = buffer.resource;
	}
	command_list->ResourceBarrier(desc.num_buffer_barriers, barriers);

	for(std::size_t i = 0; i < desc.num_texture_barriers; ++i) {
		const Texture& texture = resources.texture_pool.get(desc.textures[i]);
		barriers[i].Type = D3D12_RESOURCE_BARRIER_TYPE_UAV;
		barriers[i].UAV.pResource = texture.resource;
	}
	command_list->ResourceBarrier(desc.num_texture_barriers, barriers);
}

void CommandList::begin_render_pass(const void* command_data) {
	const BeginRenderPassDesc& begin_render_pass = *static_cast<const BeginRenderPassDesc*>(command_data);

	RenderPass& render_pass = resources.render_pass_pool.get(begin_render_pass.render_pass.handle);

	for(std::size_t i = 0; i < render_pass.num_render_targets; ++i) {
		CD_ASSERT(begin_render_pass.color[i].dimension == TextureViewDimension::Texture2D);

		Texture& render_target = resources.texture_pool.get(begin_render_pass.color[i].texture);


		if(invalid_cpu_handle(render_target.rtv)) {
			D3D12_RENDER_TARGET_VIEW_DESC view_desc {};
			view_desc.Format = dxgi_format(begin_render_pass.color[i].format);
			view_desc.ViewDimension = D3D12_RTV_DIMENSION_TEXTURE2D;
			view_desc.Texture2D.MipSlice = begin_render_pass.color[i].mip_level;
			
			CPUHandle handle = rtv_pool.add_descriptor();
			adapter.device->CreateRenderTargetView(render_target.resource, &view_desc, handle);
			render_target.rtv = handle;
		}


		render_pass.render_targets[i].cpuDescriptor = render_target.rtv;
	}

	D3D12_RENDER_PASS_DEPTH_STENCIL_DESC* depth_stencil = render_pass.depth_stencil_enable ? &render_pass.depth_stencil_target : nullptr;
	if(depth_stencil) {
		CD_ASSERT(begin_render_pass.depth_stencil_target.dimension == TextureViewDimension::Texture2D);


		Texture& depth_stencil_target = resources.texture_pool.get(begin_render_pass.depth_stencil_target.texture);

		D3D12_DSV_FLAGS dsv_flags = D3D12_DSV_FLAG_NONE;
		if(!begin_render_pass.depth_write) {
			dsv_flags |= D3D12_DSV_FLAG_READ_ONLY_DEPTH;
		}
		if(!begin_render_pass.stencil_write) {
			dsv_flags |= D3D12_DSV_FLAG_READ_ONLY_STENCIL;
		}

		CPUHandle* dsv_handle = begin_render_pass.depth_write ? &depth_stencil_target.dsv_write : &depth_stencil_target.dsv_read;

		if(invalid_cpu_handle(*dsv_handle)) {
			*dsv_handle = dsv_pool.add_descriptor();

			D3D12_DEPTH_STENCIL_VIEW_DESC dsv_desc {
				dxgi_format(begin_render_pass.depth_stencil_target.format),
				D3D12_DSV_DIMENSION_TEXTURE2D,
				dsv_flags
			};

			adapter.device->CreateDepthStencilView(depth_stencil_target.resource, &dsv_desc, *dsv_handle);
		}

		depth_stencil->cpuDescriptor = *dsv_handle;
	}

	issue_barriers();
	command_list->BeginRenderPass(render_pass.num_render_targets, render_pass.render_targets, depth_stencil, D3D12_RENDER_PASS_FLAG_NONE);

	current_render_pass = &render_pass;

	D3D12_VIEWPORT viewport {
		begin_render_pass.viewport.top_left_x,
		begin_render_pass.viewport.top_left_y,
		begin_render_pass.viewport.width,
		begin_render_pass.viewport.height,
		begin_render_pass.viewport.min_z,
		begin_render_pass.viewport.max_z
	};

	command_list->RSSetViewports(1, &viewport);
	command_list->IASetPrimitiveTopology(d3d12_primitive_topology[static_cast<std::size_t>(begin_render_pass.primitive_topology)]);
}

void CommandList::end_render_pass() {
	CD_ASSERT(current_render_pass);

	command_list->EndRenderPass();
	current_render_pass = nullptr;
}

void CommandList::copy_buffer(const void* command_data) {
	const CopyBufferDesc& copy = *static_cast<const CopyBufferDesc*>(command_data);

	const Buffer& dst_buffer = resources.buffer_pool.get(copy.dst);
	const Buffer& src_buffer = resources.buffer_pool.get(copy.src);

	issue_barriers();
	command_list->CopyBufferRegion(dst_buffer.resource, copy.dst_offset, src_buffer.resource, copy.src_offset, copy.num_bytes);
}

void CommandList::copy_texture(const void* command_data) {
	const CopyTextureDesc& copy = *static_cast<const CopyTextureDesc*>(command_data);

	const Texture& texture_dst = resources.texture_pool.get(copy.dst.texture);
	D3D12_TEXTURE_COPY_LOCATION dst {texture_dst.resource, D3D12_TEXTURE_COPY_TYPE_SUBRESOURCE_INDEX};
	dst.SubresourceIndex = texture_subresource(copy.dst.mip_level, copy.dst.index, copy.dst.plane, copy.dst.mip_count, copy.dst.depth);

	const Texture& texture_src = resources.texture_pool.get(copy.src.texture);
	D3D12_TEXTURE_COPY_LOCATION src {texture_src.resource, D3D12_TEXTURE_COPY_TYPE_SUBRESOURCE_INDEX};
	src.SubresourceIndex = texture_subresource(copy.src.mip_level, copy.src.index, copy.src.plane, copy.src.mip_count, copy.src.depth);

	issue_barriers();
	command_list->CopyTextureRegion(&dst, 0, 0, 0, &src, nullptr);
}

void CommandList::copy_buffer_to_texture(const void* command_data) {
	const CopyBufferToTextureDesc& copy = *static_cast<const CopyBufferToTextureDesc*>(command_data);

	const Texture& texture = resources.texture_pool.get(copy.texture.texture);
	D3D12_TEXTURE_COPY_LOCATION dst {texture.resource, D3D12_TEXTURE_COPY_TYPE_SUBRESOURCE_INDEX};
	dst.SubresourceIndex = texture_subresource(copy.texture.mip_level, copy.texture.index, copy.texture.plane, copy.texture.mip_count, copy.texture.depth);

	const Buffer& buffer = resources.buffer_pool.get(copy.buffer);
	D3D12_TEXTURE_COPY_LOCATION src {};
	src.pResource = buffer.resource;
	src.Type = D3D12_TEXTURE_COPY_TYPE_PLACED_FOOTPRINT;
	src.PlacedFootprint.Offset = copy.buffer_offset;
	src.PlacedFootprint.Footprint.Format = dxgi_format(copy.texture.format);
	src.PlacedFootprint.Footprint.Width = copy.width;
	src.PlacedFootprint.Footprint.Height = copy.height;
	src.PlacedFootprint.Footprint.Depth = 1;
	src.PlacedFootprint.Footprint.RowPitch = copy.row_size;

	issue_barriers();
	command_list->CopyTextureRegion(&dst, 0, 0, 0, &src, nullptr);
}

void CommandList::copy_texture_to_buffer(const void* command_data) {
	const CopyTextureToBufferDesc& copy = *static_cast<const CopyTextureToBufferDesc*>(command_data);

	const Buffer& buffer = resources.buffer_pool.get(copy.buffer);
	D3D12_TEXTURE_COPY_LOCATION dst {};
	dst.pResource = buffer.resource;
	dst.Type = D3D12_TEXTURE_COPY_TYPE_PLACED_FOOTPRINT;
	dst.PlacedFootprint.Offset = copy.buffer_offset;
	dst.PlacedFootprint.Footprint.Format = dxgi_format(copy.texture.format);
	dst.PlacedFootprint.Footprint.Width = copy.width;
	dst.PlacedFootprint.Footprint.Height = copy.height;
	dst.PlacedFootprint.Footprint.Depth = 1;
	dst.PlacedFootprint.Footprint.RowPitch = copy.row_size;

	const Texture& texture = resources.texture_pool.get(copy.texture.texture);
	D3D12_TEXTURE_COPY_LOCATION src {texture.resource, D3D12_TEXTURE_COPY_TYPE_SUBRESOURCE_INDEX};
	src.SubresourceIndex = texture_subresource(copy.texture.mip_level, copy.texture.index, copy.texture.plane, copy.texture.mip_count, copy.texture.depth);

	issue_barriers();
	command_list->CopyTextureRegion(&dst, 0, 0, 0, &src, nullptr);
}

void CommandList::dispatch(const void* command_data) {
	const DispatchDesc& desc = *static_cast<const DispatchDesc*>(command_data);

	issue_barriers();

	const PipelineState& state = resources.pipeline_state_pool.get(desc.compute_pipeline.handle);
	set_compute_state(state, desc.pipeline_input_state);

	command_list->Dispatch(desc.x, desc.y, desc.z);
}

void CommandList::dispatch_indirect(const void* command_data, ID3D12CommandSignature* dispatch_indirect_signature) {
	const DispatchIndirectDesc& desc = *static_cast<const DispatchIndirectDesc*>(command_data);

	issue_barriers();

	const PipelineState& state = resources.pipeline_state_pool.get(desc.compute_pipeline.handle);
	set_compute_state(state, desc.pipeline_input_state);

	const Buffer& args_buffer = resources.buffer_pool.get(desc.args);
	command_list->ExecuteIndirect(dispatch_indirect_signature, 1, args_buffer.resource, desc.offset, 0, 0);
}

void CommandList::draw(const void* command_data) {
	const DrawDesc& desc = *static_cast<const DrawDesc*>(command_data);

	const PipelineState& state = resources.pipeline_state_pool.get(desc.graphics_pipeline.handle);
	set_graphics_state(state, desc.pipeline_input_state);

	D3D12_VERTEX_BUFFER_VIEW vbv[max_vertex_buffers] {};
	if(desc.input_buffer != BufferHandle::Null) {
		const Buffer& input_buffer = resources.buffer_pool.get(desc.input_buffer);
		GPUVA vertex_va = input_buffer.va;

		for(std::size_t i = 0; i < desc.num_vertex_buffers; ++i) {
			vbv[i].BufferLocation = vertex_va + desc.vertex_buffer_offsets[i];
			vbv[i].SizeInBytes = desc.vertex_buffer_sizes[i];
			vbv[i].StrideInBytes = desc.vertex_buffer_strides[i];
		}
	}

	D3D12_RECT scissor {
		static_cast<LONG>(desc.rect.left),
		static_cast<LONG>(desc.rect.top),
		static_cast<LONG>(desc.rect.right),
		static_cast<LONG>(desc.rect.bottom),
	};

	command_list->RSSetScissorRects(1, &scissor);

	command_list->IASetVertexBuffers(0, desc.num_vertex_buffers, vbv);

	if(desc.index_buffer != BufferHandle::Null) {
		const Buffer& index_buffer = resources.buffer_pool.get(desc.index_buffer);
		GPUVA index_va = index_buffer.va + desc.index_buffer_offset;

		D3D12_INDEX_BUFFER_VIEW ibv {
			index_va,
			desc.index_buffer_size,
			DXGI_FORMAT_R32_UINT
		};
		command_list->IASetIndexBuffer(&ibv);
		command_list->DrawIndexedInstanced(desc.num_elements, desc.num_instances, 0, 0, 0);
	}
	else {
		command_list->DrawInstanced(desc.num_elements, desc.num_instances, 0, 0);
	}
}

void CommandList::copy_to_swapchain(const void* command_data, const SwapChain& swapchain) {
	const CopyToSwapChainDesc& copy = *static_cast<const CopyToSwapChainDesc*>(command_data);

	ID3D12Resource* swapchain_surface = swapchain.resources[swapchain.dxgi_swapchain->GetCurrentBackBufferIndex()];

	D3D12_TEXTURE_COPY_LOCATION dst {swapchain_surface, D3D12_TEXTURE_COPY_TYPE_SUBRESOURCE_INDEX};

	const Texture& texture_src = resources.texture_pool.get(copy.texture.texture);
	D3D12_TEXTURE_COPY_LOCATION src {texture_src.resource, D3D12_TEXTURE_COPY_TYPE_SUBRESOURCE_INDEX};
	src.SubresourceIndex = texture_subresource(copy.texture.mip_level, copy.texture.index, copy.texture.plane, copy.texture.mip_count, copy.texture.depth);

	add_transition(texture_src.resource, src.SubresourceIndex, d3d12_resource_state(copy.texture_state), D3D12_RESOURCE_STATE_COPY_SOURCE);
	add_transition(swapchain_surface, 0, D3D12_RESOURCE_STATE_COMMON, D3D12_RESOURCE_STATE_COPY_DEST);
	issue_barriers();

	command_list->CopyTextureRegion(&dst, 0, 0, 0, &src, nullptr);

	add_transition(swapchain_surface, 0, D3D12_RESOURCE_STATE_COPY_DEST, D3D12_RESOURCE_STATE_COMMON);
	add_transition(texture_src.resource, src.SubresourceIndex, D3D12_RESOURCE_STATE_COPY_SOURCE, d3d12_resource_state(copy.texture_state));
	issue_barriers();
}

void CommandList::insert_timestamp(const void* command_data, ID3D12QueryHeap* query_heap) {
	const InsertTimestampDesc& timestamp = *static_cast<const InsertTimestampDesc*>(command_data);

	CD_ASSERT(timestamp.index < max_timestamp_queries);
	command_list->EndQuery(query_heap, D3D12_QUERY_TYPE_TIMESTAMP, timestamp.index);
}

void CommandList::resolve_timestamps(const void* command_data, ID3D12QueryHeap* query_heap) {
	const ResolveTimestampsDesc& resolve = *static_cast<const ResolveTimestampsDesc*>(command_data);

	CD_ASSERT(resolve.index + resolve.timestamp_count < max_timestamp_queries);
	CD_ASSERT(resolve.dest != BufferHandle::Null);
	const Buffer& buffer = resources.buffer_pool.get(resolve.dest);

	command_list->ResolveQueryData(query_heap, D3D12_QUERY_TYPE_TIMESTAMP, resolve.index, resolve.timestamp_count, buffer.resource, resolve.aligned_offset);
}

void CommandList::issue_barriers() {
	if(barrier_buffer.barrier_count) {
		command_list->ResourceBarrier(barrier_buffer.barrier_count, barrier_buffer.barriers);
		barrier_buffer.barrier_count = 0;
	}
}

void CommandList::set_compute_state(const PipelineState& state, const PipelineInputState& rs_state) {
	if(ID3D12RootSignature* rs = state.root_signature; rs != compute_pso.root_signature) {
		command_list->SetComputeRootSignature(state.root_signature);
		compute_pso.root_signature = rs;
		compute_arguments.cleared = true;
	}

	for(std::uint32_t i = 0; i < rs_state.num_elements; ++i) {
		switch(rs_state.types[i]) {
		case PipelineInputGroupType::ResourceList: {
			if(!compute_arguments.cleared && rs_state.input_elements[i].resource_list == compute_arguments.arguments.input_elements[i].resource_list) {
				continue;
			}

			DescriptorTable& table = resources.descriptor_table_pool.get(rs_state.input_elements[i].resource_list.handle);
			command_list->SetComputeRootDescriptorTable(i, table.gpu_start);
			break;
		}
		case PipelineInputGroupType::Buffer: {
			if(!compute_arguments.cleared && rs_state.input_elements[i].buffer == compute_arguments.arguments.input_elements[i].buffer) {
				continue;
			}

			Buffer& buffer = resources.buffer_pool.get(rs_state.input_elements[i].buffer.buffer);
			switch(rs_state.input_elements[i].buffer.type) {
			case DescriptorType::CBV:
				command_list->SetComputeRootConstantBufferView(i, buffer.va + rs_state.input_elements[i].buffer.offset);
				break;
			case DescriptorType::SRV:
				command_list->SetComputeRootShaderResourceView(i, buffer.va + rs_state.input_elements[i].buffer.offset);
				break;
			case DescriptorType::UAV:
				command_list->SetComputeRootUnorderedAccessView(i, buffer.va + rs_state.input_elements[i].buffer.offset);
				break;
			}
			break;
		}
		default:
			CD_FAIL("unhandled type");
		}
	}

	compute_arguments.cleared = false;

	if(ID3D12PipelineState* pso = state.pso; pso != compute_pso.pso) {
		command_list->SetPipelineState(pso);
		compute_pso.pso = pso;
	}
}

void CommandList::set_graphics_state(const PipelineState& state, const PipelineInputState& rs_state) {
	if(ID3D12RootSignature* rs = state.root_signature; rs != graphics_pso.root_signature) {
		command_list->SetGraphicsRootSignature(state.root_signature);
		graphics_pso.root_signature = rs;
		graphics_arguments.cleared = true;
	}

	for(std::uint32_t i = 0; i < rs_state.num_elements; ++i) {
		switch(rs_state.types[i]) {
		case PipelineInputGroupType::ResourceList: {
			if(!graphics_arguments.cleared && rs_state.input_elements[i].resource_list == graphics_arguments.arguments.input_elements[i].resource_list) {
				continue;
			}

			DescriptorTable& table = resources.descriptor_table_pool.get(rs_state.input_elements[i].resource_list.handle);
			command_list->SetGraphicsRootDescriptorTable(i, table.gpu_start);
			break;
		}
		case PipelineInputGroupType::Buffer: {
			if(!graphics_arguments.cleared && rs_state.input_elements[i].buffer == graphics_arguments.arguments.input_elements[i].buffer) {
				continue;
			}

			Buffer& buffer = resources.buffer_pool.get(rs_state.input_elements[i].buffer.buffer);
			switch(rs_state.input_elements[i].buffer.type) {
			case DescriptorType::CBV:
				command_list->SetGraphicsRootConstantBufferView(i, buffer.va + rs_state.input_elements[i].buffer.offset);
				break;
			case DescriptorType::SRV:
				command_list->SetGraphicsRootShaderResourceView(i, buffer.va + rs_state.input_elements[i].buffer.offset);
				break;
			case DescriptorType::UAV:
				command_list->SetGraphicsRootUnorderedAccessView(i, buffer.va + rs_state.input_elements[i].buffer.offset);
				break;
			}
			break;
		}
		default:
			CD_FAIL("unhandled type");
		}
	}

	graphics_arguments.cleared = false;

	if(ID3D12PipelineState* pso = state.pso; pso != graphics_pso.pso) {
		command_list->SetPipelineState(pso);
		graphics_pso.pso = pso;
	}
}

void CommandList::add_transition(ID3D12Resource* resource, std::uint32_t index, D3D12_RESOURCE_STATES before, D3D12_RESOURCE_STATES after) {
	if(barrier_buffer.barrier_count == std::size(barrier_buffer.barriers) - 1) {
		issue_barriers();
	}
	barrier_buffer.barriers[barrier_buffer.barrier_count] = get_resource_transition(resource, index, before, after);
	++barrier_buffer.barrier_count;
}

Engine::Engine(const Adapter& adapter, DeviceResources& resources, const ShaderDescriptorHeap& descriptor_heap) :
	adapter(adapter),
	resources(resources),
	swapchain(nullptr),
	descriptor_heap(descriptor_heap),
	timing_heap(nullptr),
	dispatch_indirect_signature(nullptr) {
	for(std::size_t type = 0; type < CommandQueueType_Count; ++type) {
		ID3D12Fence* fence = nullptr;
		HR_ASSERT(adapter.device->CreateFence(0, D3D12_FENCE_FLAG_NONE, IID_PPV_ARGS(&fence)));
		queues[type].fence = {fence, 0, 0};

		D3D12_COMMAND_QUEUE_DESC queue_desc {};
		queue_desc.Flags = D3D12_COMMAND_QUEUE_FLAG_NONE;
		queue_desc.NodeMask = 1 << adapter.node_index;
		queue_desc.Type = d3d12_command_list_type(static_cast<CommandQueueType>(type));

		HR_ASSERT(adapter.device->CreateCommandQueue(&queue_desc, IID_PPV_ARGS(&queues[type].queue)));
	}

	{
		D3D12_QUERY_HEAP_DESC timing_heap_desc {};
		timing_heap_desc.Count = max_timestamp_queries;
		timing_heap_desc.NodeMask = 1 << adapter.node_index;
		timing_heap_desc.Type = D3D12_QUERY_HEAP_TYPE_TIMESTAMP;
		HR_ASSERT(adapter.device->CreateQueryHeap(&timing_heap_desc, IID_PPV_ARGS(&timing_heap)));
	}

	{
		D3D12_COMMAND_SIGNATURE_DESC dispatch_indirect_desc {};
		dispatch_indirect_desc.ByteStride = 12;
		dispatch_indirect_desc.NodeMask = 1 << adapter.node_index;
		dispatch_indirect_desc.NumArgumentDescs = 1;
		D3D12_INDIRECT_ARGUMENT_DESC dispatch_arg {D3D12_INDIRECT_ARGUMENT_TYPE_DISPATCH};
		dispatch_indirect_desc.pArgumentDescs = &dispatch_arg;
		HR_ASSERT(adapter.device->CreateCommandSignature(&dispatch_indirect_desc, nullptr, IID_PPV_ARGS(&dispatch_indirect_signature)));
	}
}

Engine::~Engine() {
	sync();

	for(std::size_t type = 0; type < CommandQueueType_Count; ++type) {
		queues[type].fence.fence->Release();
		queues[type].queue->Release();
	}

	dispatch_indirect_signature->Release();
	timing_heap->Release();
}

Signal Engine::submit_command_buffer(const CommandBuffer& cb, CommandQueueType queue_type) {
	CommandList& command_list = get_command_list(queue_type);

	for(const std::uint8_t* ptr = cb.get_commands(); ptr < cb.end(); ptr += cb.get_command_size(ptr)) {
		CommandType type = cb.get_command_type(ptr);

		switch(type) {
		case CommandType::Draw:
			command_list.draw(ptr);
			break;
		case CommandType::LayoutBarrier:
			command_list.transition_barrier(ptr);
			break;
		case CommandType::ResourceBarrier:
			command_list.uav_barrier(ptr);
			break;
		case CommandType::Dispatch:
			command_list.dispatch(ptr);
			break;
		case CommandType::DispatchIndirect:
			command_list.dispatch_indirect(ptr, dispatch_indirect_signature);
			break;
		case CommandType::BeginRenderPass:
			command_list.begin_render_pass(ptr);
			break;
		case CommandType::EndRenderPass:
			command_list.end_render_pass();
			break;
		case CommandType::CopyBuffer:
			command_list.copy_buffer(ptr);
			break;
		case CommandType::CopyBufferToTexture:
			command_list.copy_buffer_to_texture(ptr);
			break;
		case CommandType::CopyTexture:
			command_list.copy_texture(ptr);
			break;
		case CommandType::CopyTextureToBuffer:
			command_list.copy_texture_to_buffer(ptr);
			break;
		case CommandType::InsertTimestamp:
			command_list.insert_timestamp(ptr, timing_heap);
			break;
		case CommandType::ResolveTimestamps:
			command_list.resolve_timestamps(ptr, timing_heap);
			break;
		case CommandType::CopyToSwapChain:
			command_list.copy_to_swapchain(ptr, *swapchain);
			break;
		default:
			CD_FAIL("unhandled type");
		}
	}

	++queues[queue_type].fence.head;

	command_list.close();

	queues[queue_type].command_list_buffer.emplace_back(command_list.d3d12_command_list());

	if(queue_type == CommandQueueType_Compute || queue_type == CommandQueueType_Copy) {
		flush_queue(queue_type);
	}

	return {queue_type, queues[queue_type].fence.head};
}

void Engine::block(const Signal& fence) {
	flush_queue(fence.queue);
	signal_queue(fence.queue);

	CommandQueue& queue = queues[fence.queue];
	CD_ASSERT(fence.value <= queue.fence.last_signal);

	HR_ASSERT(queue.fence.fence->SetEventOnCompletion(queue.fence.head, nullptr));
	queue.fence.tail = queue.fence.head;
}

Signal Engine::present() {
	CD_ASSERT(swapchain);

	flush_queue(CommandQueueType_Direct);

	UINT index = swapchain->dxgi_swapchain->GetCurrentBackBufferIndex();
	CPUHandle handle = swapchain->buffers[index];
	HR_ASSERT(swapchain->dxgi_swapchain->Present(0, 0));

	Fence& fence = queues[CommandQueueType_Direct].fence;
	++fence.head;
	signal_queue(CommandQueueType_Direct);

	for(CommandQueue& queue : queues) {
		queue.fence.tail = queue.fence.fence->GetCompletedValue();
	}

	return {CommandQueueType_Direct, fence.head};
}

void Engine::set_swapchain(const SwapChain* sw) {
	swapchain = sw;
}

ID3D12CommandQueue* Engine::get_queue(CommandQueueType type) const {
	return queues[type].queue;
}

void Engine::signal_queue(CommandQueueType type) {
	CommandQueue& queue = queues[type];
	Fence& fence = queue.fence;
	if(fence.last_signal < fence.head) {
		HR_ASSERT(queue.queue->Signal(fence.fence, fence.head));
		fence.last_signal = fence.head;
	}
}

void Engine::wait(const Signal& signal, CommandQueueType type) {
	const CommandQueue& producer = queues[signal.queue];
	const CommandQueue& target = queues[type];

	CD_ASSERT(producer.fence.head >= signal.value);

	signal_queue(signal.queue);
	flush_queue(type);

	HR_ASSERT(target.queue->Wait(producer.fence.fence, producer.fence.head));
}

void Engine::flush() {
	for(std::size_t type = 0; type < CommandQueueType_Count; ++type) {
		flush_queue(static_cast<CommandQueueType>(type));
	}
}

void Engine::flush_queue(CommandQueueType type) {
	CommandQueue& queue = queues[type];
	if(std::size_t size = queue.command_list_buffer.size()) {
		queue.queue->ExecuteCommandLists(static_cast<std::uint32_t>(size), queue.command_list_buffer.data());

		for(auto& command_list : command_list_pool[type]) {
			if(command_list->get_state() == CommandListState::Pending) {
				command_list->reset();
			}
		}

		queue.command_list_buffer.clear();
	}
}

void Engine::sync() {
	flush();

	for(std::size_t i = 0; i < CommandQueueType_Count; ++i) {
		Fence& fence = queues[i].fence;
		queues[i].queue->Signal(fence.fence, fence.head);
		HR_ASSERT(fence.fence->SetEventOnCompletion(fence.head, nullptr));
		fence.tail = fence.head;
	}
}

}