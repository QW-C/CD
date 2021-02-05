#include <CD/GPU/D3D12/Device.hpp>

namespace CD::GPU::D3D12 {

Device::Device(Adapter& adapter, const SwapChainDesc* swapchain_desc, IDXGIFactory7* factory) :
	adapter(adapter),
	allocator(adapter),
	shader_descriptor_heap(adapter),
	engine(adapter, resources, shader_descriptor_heap),
	rtv_pool(adapter, swapchain_backbuffer_count, D3D12_DESCRIPTOR_HEAP_TYPE_RTV) {

	if(swapchain_desc) {
		DXGI_SWAP_CHAIN_DESC1 desc {};
		desc.BufferCount = swapchain_backbuffer_count;
		desc.Width = swapchain_desc->width;
		desc.Height = swapchain_desc->height;
		desc.Format = DXGI_FORMAT_R8G8B8A8_UNORM;
		desc.BufferUsage = DXGI_USAGE_RENDER_TARGET_OUTPUT;
		desc.SwapEffect = DXGI_SWAP_EFFECT_FLIP_DISCARD;
		desc.SampleDesc = {1, 0};
		desc.Flags = DXGI_SWAP_CHAIN_FLAG_ALLOW_MODE_SWITCH;
		desc.Scaling = DXGI_SCALING_STRETCH;
		desc.AlphaMode = DXGI_ALPHA_MODE_IGNORE;

		DXGI_SWAP_CHAIN_FULLSCREEN_DESC fullscreen_desc {};
		fullscreen_desc.Windowed = true;

		swapchain = std::make_unique<SwapChain>();

		IDXGISwapChain1* sc = nullptr;
		ID3D12CommandQueue* queue = engine.get_queue(CommandQueueType_Direct);

		HR_ASSERT(factory->CreateSwapChainForHwnd(queue, static_cast<HWND>(swapchain_desc->handle), &desc, &fullscreen_desc, nullptr, &sc));
		HR_ASSERT(sc->QueryInterface<IDXGISwapChain3>(&swapchain->dxgi_swapchain));
		sc->Release();

		for(std::uint32_t i = 0; i < swapchain_backbuffer_count; ++i) {
			ID3D12Resource* surface = nullptr;
			HR_ASSERT(swapchain->dxgi_swapchain->GetBuffer(i, IID_PPV_ARGS(&surface)));
			swapchain->resources[i] = surface;
			swapchain->buffers[i] = rtv_pool.add_descriptor();
			D3D12_RENDER_TARGET_VIEW_DESC rtv_desc {};
			rtv_desc.Format = DXGI_FORMAT_R8G8B8A8_UNORM;
			rtv_desc.ViewDimension = D3D12_RTV_DIMENSION_TEXTURE2D;
			adapter.device->CreateRenderTargetView(surface, &rtv_desc, swapchain->buffers[i]);

			engine.set_swapchain(swapchain.get());
		}
	}

	for(std::size_t type = CommandQueueType_Direct; type < CommandQueueType_Count; ++type) {
		engine.get_queue(static_cast<CommandQueueType>(type))->GetTimestampFrequency(&adapter.feature_info.timestamp_frequency[type]);
	}

	resources.buffer_pool.add({});
	resources.texture_pool.add({});
	resources.descriptor_table_pool.add({});
	resources.pipeline_state_pool.add({});
	resources.render_pass_pool.add({});
}

Device::~Device() = default;

BufferHandle Device::create_buffer(const BufferDesc& buffer_desc) {
	D3D12_RESOURCE_DESC desc {};
	desc.Dimension = D3D12_RESOURCE_DIMENSION_BUFFER;
	desc.Width = buffer_desc.size;
	desc.Height = 1;
	desc.DepthOrArraySize = 1;
	desc.MipLevels = 1;
	desc.SampleDesc = {1, 0};
	desc.Layout = D3D12_TEXTURE_LAYOUT_ROW_MAJOR;

	if(buffer_desc.flags & BindFlags_RW) {
		desc.Flags |= D3D12_RESOURCE_FLAG_ALLOW_UNORDERED_ACCESS;
	}

	Buffer buffer = allocator.create_buffer(desc, d3d12_heap_type(buffer_desc.storage), d3d12_initial_state(buffer_desc.storage));
	return static_cast<BufferHandle>(resources.buffer_pool.add(buffer));
}

TextureHandle Device::create_texture(const TextureDesc& texture_desc) {
	D3D12_RESOURCE_DESC desc {};
	desc.Dimension = d3d12_resource_dimension(texture_desc.dimension);
	desc.Width = texture_desc.width;
	desc.Height = texture_desc.height;
	desc.DepthOrArraySize = texture_desc.depth;
	desc.MipLevels = texture_desc.mip_levels;
	desc.Format = dxgi_format(texture_desc.format);
	if(texture_desc.flags & BindFlags_DepthStencilTarget) {
		desc.Flags |= D3D12_RESOURCE_FLAG_ALLOW_DEPTH_STENCIL;

		CD_ASSERT(!(texture_desc.flags & BindFlags_RenderTarget));
		CD_ASSERT(!(texture_desc.flags & BindFlags_RW));
	}
	if(texture_desc.flags & BindFlags_RenderTarget) {
		desc.Flags |= D3D12_RESOURCE_FLAG_ALLOW_RENDER_TARGET;
	}
	if(texture_desc.flags & BindFlags_RW) {
		desc.Flags |= D3D12_RESOURCE_FLAG_ALLOW_UNORDERED_ACCESS;
	}

	desc.SampleDesc = {texture_desc.sample_count, 0};
	desc.Layout = D3D12_TEXTURE_LAYOUT_UNKNOWN;

	Texture texture = allocator.create_texture(desc, D3D12_HEAP_TYPE_DEFAULT, D3D12_RESOURCE_STATE_COMMON);
	return static_cast<TextureHandle>(resources.texture_pool.add(texture));
}

PipelineHandle Device::create_render_pass(const RenderPassDesc& desc) {
	RenderPass render_pass {};
	for(std::size_t i = 0; i < desc.num_render_targets; ++i) {
		const RenderPassRenderTargetDesc& rt_desc = desc.render_targets[i];

		D3D12_RENDER_PASS_BEGINNING_ACCESS begin_access;
		begin_access.Type = d3d12_beginning_access_op(rt_desc.begin_op);

		if(rt_desc.begin_op == RenderPassBeginOp::Clear) {
			begin_access.Clear.ClearValue.Format = dxgi_format(rt_desc.clear_format);
			std::copy(rt_desc.clear_value, rt_desc.clear_value + 3, begin_access.Clear.ClearValue.Color);
		}

		D3D12_RENDER_PASS_ENDING_ACCESS end_access;
		end_access.Type = d3d12_ending_access_op(rt_desc.end_op);

		render_pass.num_render_targets = desc.num_render_targets;
		render_pass.render_targets[i] = {
			{},
			begin_access,
			end_access
		};
	}

	if(const RenderPassDepthStencilTargetDesc* ds = desc.depth_stencil_target) {
		render_pass.depth_stencil_enable = true;
		render_pass.depth_stencil_target = {
			{},
			d3d12_depth_stencil_beginning_access(ds->depth_begin_op, ds->depth_clear_format, ds->begin_depth_clear_value, 0),
			d3d12_depth_stencil_beginning_access(ds->stencil_begin_op, ds->stencil_clear_format, 0, ds->begin_stencil_clear_value),
			{d3d12_ending_access_op(ds->depth_end_op)},
			{d3d12_ending_access_op(ds->stencil_end_op)}
		};
	}

	return {static_cast<std::uint32_t>(resources.render_pass_pool.add(render_pass)), PipelineResourceType::RenderPass};
}

PipelineHandle Device::create_pipeline_state(const GraphicsPipelineDesc& pipeline_desc, const PipelineInputLayout& pipeline_layout) {
	CD_ASSERT(pipeline_desc.vs.bytecode);
	CD_ASSERT(pipeline_desc.render_target_count <= max_render_targets);

	D3D12_GRAPHICS_PIPELINE_STATE_DESC desc {};
	desc.VS = {pipeline_desc.vs.bytecode, pipeline_desc.vs.size};
	desc.PS = {pipeline_desc.ps.bytecode, pipeline_desc.ps.size};
	desc.DS = {pipeline_desc.ds.bytecode, pipeline_desc.ds.size};
	desc.HS = {pipeline_desc.hs.bytecode, pipeline_desc.hs.size};
	desc.GS = {pipeline_desc.gs.bytecode, pipeline_desc.gs.size};
	desc.PrimitiveTopologyType = d3d12_primitive_topology_type(pipeline_desc.primitive_topology);
	desc.NumRenderTargets = pipeline_desc.render_target_count;
	desc.SampleDesc = {pipeline_desc.sample_count, 0};
	desc.SampleMask = pipeline_desc.sample_mask;
	desc.BlendState.AlphaToCoverageEnable = pipeline_desc.alpha_to_coverage_enable;
	desc.BlendState.IndependentBlendEnable = pipeline_desc.independent_blend_enable;

	for(std::size_t i = 0; i < pipeline_desc.render_target_count; ++i) {
		desc.RTVFormats[i] = dxgi_format(pipeline_desc.render_target_format[i]);
		desc.BlendState.RenderTarget[i] = d3d12_render_target_blend_desc(pipeline_desc.blend_desc.blend[i]);
		desc.BlendState.RenderTarget[i].RenderTargetWriteMask = D3D12_COLOR_WRITE_ENABLE_ALL;
	}

	desc.RasterizerState = {
		d3d12_fill_mode(pipeline_desc.rasterizer.fill_mode),
		d3d12_cull_mode(pipeline_desc.rasterizer.cull_mode),
		pipeline_desc.rasterizer.front_face == FrontFace::Clockwise ? 0 : 1,
		pipeline_desc.rasterizer.depth_bias,
		pipeline_desc.rasterizer.depth_bias_clamp,
		pipeline_desc.rasterizer.depth_bias_slope,
		pipeline_desc.rasterizer.depth_clip_enable,
		pipeline_desc.rasterizer.multisample_enable,
		pipeline_desc.rasterizer.antialiased_line_enable,
		pipeline_desc.rasterizer.forced_sample_count,
		pipeline_desc.rasterizer.conservative_raster_enable ? D3D12_CONSERVATIVE_RASTERIZATION_MODE_ON : D3D12_CONSERVATIVE_RASTERIZATION_MODE_OFF
	};

	desc.DSVFormat = dxgi_format(pipeline_desc.depth_stencil_format);
	desc.DepthStencilState = {
		pipeline_desc.depth_stencil.depth_enable,
		pipeline_desc.depth_stencil.depth_write_mask == DepthWriteMask::All ? D3D12_DEPTH_WRITE_MASK_ALL : D3D12_DEPTH_WRITE_MASK_ZERO,
		d3d12_comparison_func[static_cast<std::size_t>(pipeline_desc.depth_stencil.depth_compare_op)],
		pipeline_desc.depth_stencil.stencil_enable,
		pipeline_desc.depth_stencil.stencil_read_mask,
		pipeline_desc.depth_stencil.stencil_write_mask,
		d3d12_depth_stencil_op(pipeline_desc.depth_stencil.front_face_op),
		d3d12_depth_stencil_op(pipeline_desc.depth_stencil.back_face_op)
	};

	desc.InputLayout.NumElements = pipeline_desc.input_layout.num_elements;
	D3D12_INPUT_ELEMENT_DESC input_elements[max_input_elements] {};
	for(std::size_t i = 0; i < pipeline_desc.input_layout.num_elements; ++i) {
		const InputElement* input_layout = pipeline_desc.input_layout.elements;
		input_elements[i] = {
			input_layout[i].name,
			input_layout[i].index,
			dxgi_format(input_layout[i].format),
			input_layout[i].slot
		};
		if(input_layout[i].instance_element) {
			input_elements[i].InputSlotClass = D3D12_INPUT_CLASSIFICATION_PER_INSTANCE_DATA;
			input_elements[i].InstanceDataStepRate = input_layout[i].instance_data_rate;
		}
	};
	desc.InputLayout.pInputElementDescs = input_elements;

	desc.NodeMask = 1 << adapter.node_index;

	desc.pRootSignature = create_root_signature(pipeline_layout, true);
	ID3D12PipelineState* pso = nullptr;
	HR_ASSERT(adapter.device->CreateGraphicsPipelineState(&desc, IID_PPV_ARGS(&pso)));

	PipelineHandle handle = {static_cast<std::uint32_t>(resources.pipeline_state_pool.add({pso, desc.pRootSignature})), PipelineResourceType::GraphicsPipeline};
	return handle;
}

PipelineHandle Device::create_pipeline_state(const ComputePipelineDesc& pipeline_desc, const PipelineInputLayout& pipeline_layout) {
	D3D12_COMPUTE_PIPELINE_STATE_DESC desc {};
	desc.pRootSignature = create_root_signature(pipeline_layout);
	desc.CS = {pipeline_desc.compute_shader.bytecode, pipeline_desc.compute_shader.size};
	desc.NodeMask = 1 << adapter.node_index;

	ID3D12PipelineState* pso = nullptr;
	HR_ASSERT(adapter.device->CreateComputePipelineState(&desc, IID_PPV_ARGS(&pso)));

	return {static_cast<std::uint32_t>(resources.pipeline_state_pool.add({pso, desc.pRootSignature})), PipelineResourceType::ComputePipeline};
}

PipelineHandle Device::create_pipeline_input_list(std::uint32_t num_descriptors) {
	DescriptorTable table = shader_descriptor_heap.create_descriptor_table(num_descriptors);
	return {static_cast<std::uint32_t>(resources.descriptor_table_pool.add(table)), PipelineResourceType::PipelineInputList};
}

void Device::destroy_buffer(BufferHandle handle) {
	Buffer& buffer = resources.buffer_pool.get(handle);
	buffer.resource->Release();
	resources.buffer_pool.remove(static_cast<std::size_t>(handle));
}

void Device::destroy_texture(TextureHandle handle) {
	Texture& texture = resources.texture_pool.get(handle);
	texture.resource->Release();
	resources.texture_pool.remove(static_cast<std::size_t>(handle));
}

void Device::destroy_pipeline_resource(PipelineHandle resource) {
	switch(resource.type) {
	case PipelineResourceType::ComputePipeline:
	case PipelineResourceType::GraphicsPipeline: {
		PipelineState& pso = resources.pipeline_state_pool.get(resource.handle);
		pso.pso->Release();
		pso.root_signature->Release();
		pso = {};
		resources.pipeline_state_pool.remove(resource.handle);
		break;
	}
	case PipelineResourceType::PipelineInputList: {
		DescriptorTable& table = resources.descriptor_table_pool.get(resource.handle);
		shader_descriptor_heap.destroy_descriptor_table(table);
		resources.descriptor_table_pool.remove(resource.handle);
		break;
	}
	case PipelineResourceType::RenderPass: {
		RenderPass& render_pass = resources.render_pass_pool.get(resource.handle);
		render_pass = {};
		resources.render_pass_pool.remove(resource.handle);
		break;
	}
	default:
		CD_FAIL("invalid resource type");
	}
}

void Device::map_buffer(BufferHandle handle, void** data, std::uint64_t offset, std::uint64_t size) {
	Buffer& buffer = resources.buffer_pool.get(handle);
	CD_ASSERT(buffer.resource);
	D3D12_RANGE range {offset, offset + size};
	buffer.resource->Map(0, &range, data);
}

void Device::unmap_buffer(BufferHandle handle, std::uint64_t offset, std::uint64_t size) {
	Buffer& buffer = resources.buffer_pool.get(handle);
	CD_ASSERT(buffer.resource);
	D3D12_RANGE range {offset, offset + size};
	buffer.resource->Unmap(0, &range);
}

void Device::update_pipeline_input_list(PipelineHandle handle, DescriptorType type, const TextureView* views, std::uint64_t num_textures, std::uint64_t offset) {
	CD_ASSERT(handle.type == PipelineResourceType::PipelineInputList);

	DescriptorTable& list = resources.descriptor_table_pool.get(handle.handle);
	CD_ASSERT(list.gpu_start.ptr != 0);
	CD_ASSERT(num_textures <= list.num_descriptors);

	std::uint32_t increment = shader_descriptor_heap.get_increment();
	CPUHandle start = {list.cpu_start.ptr + offset * increment};
	for(std::size_t i = 0; i < num_textures; ++i) {
		Texture& texture = resources.texture_pool.get(views[i].texture);
		CPUHandle cpu_handle = {start.ptr + i * increment};
		switch(type) {
		case DescriptorType::SRV: {
			D3D12_SHADER_RESOURCE_VIEW_DESC srv = srv_desc_texture(views[i]);
			adapter.device->CreateShaderResourceView(texture.resource, &srv, cpu_handle);
			break;
		}
		case DescriptorType::UAV: {
			D3D12_UNORDERED_ACCESS_VIEW_DESC uav = uav_desc_texture(views[i]);
			adapter.device->CreateUnorderedAccessView(texture.resource, nullptr, &uav, cpu_handle);
			break;
		}
		default:
			CD_FAIL("invalid descriptor type");
		}
	}
}

void Device::update_pipeline_input_list(PipelineHandle handle, DescriptorType type, const BufferView* views, std::uint64_t num_buffers, std::uint64_t offset) {
	CD_ASSERT(handle.type == PipelineResourceType::PipelineInputList);
	CD_ASSERT(type <= DescriptorType::CBV);

	const DescriptorTable& list = resources.descriptor_table_pool.get(handle.handle);
	CD_ASSERT(list.gpu_start.ptr != 0);
	CD_ASSERT(num_buffers <= list.num_descriptors);

	std::uint32_t increment = shader_descriptor_heap.get_increment();
	CPUHandle start = {list.cpu_start.ptr + offset * increment};
	for(std::size_t i = 0; i < num_buffers; ++i) {
		const Buffer& buffer = resources.buffer_pool.get(views[i].buffer);
		CPUHandle cpu_handle = {start.ptr + i * increment};
		switch(type) {
		case DescriptorType::SRV: {
			D3D12_SHADER_RESOURCE_VIEW_DESC srv {};
			srv.Format = dxgi_format(views[i].format);
			srv.Shader4ComponentMapping = D3D12_DEFAULT_SHADER_4_COMPONENT_MAPPING;
			srv.ViewDimension = D3D12_SRV_DIMENSION_BUFFER;
			srv.Buffer.FirstElement = views[i].offset;
			srv.Buffer.StructureByteStride = views[i].stride;
			srv.Buffer.NumElements = views[i].size;
			adapter.device->CreateShaderResourceView(buffer.resource, &srv, cpu_handle);
			break;
		}
		case DescriptorType::UAV: {
			D3D12_UNORDERED_ACCESS_VIEW_DESC uav {};
			uav.Format = dxgi_format(views[i].format);
			uav.ViewDimension = D3D12_UAV_DIMENSION_BUFFER;
			uav.Buffer.FirstElement = views[i].offset;
			uav.Buffer.StructureByteStride = views[i].stride;
			adapter.device->CreateUnorderedAccessView(buffer.resource, nullptr, &uav, cpu_handle);
			break;
		}
		case DescriptorType::CBV: {
			D3D12_CONSTANT_BUFFER_VIEW_DESC cbv {};
			cbv.BufferLocation = buffer.resource->GetGPUVirtualAddress() + views[i].offset;
			cbv.SizeInBytes = views[i].size;
			adapter.device->CreateConstantBufferView(&cbv, cpu_handle);
			break;
		}
		default:
			CD_FAIL("invalid descriptor type");
		}
	}
}

void Device::signal(CommandQueueType type) {
	engine.signal_queue(type);
}

void Device::wait(const Signal& producer, CommandQueueType queue) {
	engine.wait(producer, queue);
}

void Device::wait_for_fence(const Signal& producer) {
	engine.block(producer);
}

Signal Device::submit_commands(const CommandBuffer& commands, CommandQueueType type) {
	CD_ASSERT(commands.get_command_count());

	return engine.submit_command_buffer(commands, type);
}

Signal Device::reset() {
	return engine.present();
}

void Device::resize_buffers(std::uint32_t width, std::uint32_t height) {
	engine.sync();

	HR_ASSERT(swapchain->dxgi_swapchain->ResizeBuffers(swapchain_backbuffer_count, width, height, DXGI_FORMAT_R8G8B8A8_UNORM, 0));
	for(std::uint32_t i = 0; i < swapchain_backbuffer_count; ++i) {
		HR_ASSERT(swapchain->dxgi_swapchain->GetBuffer(i, IID_PPV_ARGS(&swapchain->resources[i])));
		D3D12_RENDER_TARGET_VIEW_DESC rtv_desc {};
		rtv_desc.Format = DXGI_FORMAT_R8G8B8A8_UNORM;
		rtv_desc.ViewDimension = D3D12_RTV_DIMENSION_TEXTURE2D;
		adapter.device->CreateRenderTargetView(swapchain->resources[i], &rtv_desc, swapchain->buffers[i]);
	}
}

DeviceFeatureInfo Device::report_feature_info() {
	return adapter.feature_info;
}

ID3D12RootSignature* Device::create_root_signature(const PipelineInputLayout& layout, bool ia) {
	CD_ASSERT(layout.num_entries <= max_pipeline_layout_entries);
	CD_ASSERT(layout.num_samplers <= max_pipeline_layout_samplers);

	D3D12_VERSIONED_ROOT_SIGNATURE_DESC desc {};
	desc.Version = D3D_ROOT_SIGNATURE_VERSION_1_1;
	if(ia) {
		desc.Desc_1_1.Flags = D3D12_ROOT_SIGNATURE_FLAG_ALLOW_INPUT_ASSEMBLER_INPUT_LAYOUT;
	}

	D3D12_ROOT_PARAMETER1 parameters[max_pipeline_layout_entries] {};
	D3D12_DESCRIPTOR_RANGE1 ranges[max_pipeline_layout_entries][max_resource_list_ranges] {};
	for(std::size_t i = 0; i < layout.num_entries; ++i) {
		const PipelineInputGroup& entry = layout.entries[i];

		parameters[i].ShaderVisibility = D3D12_SHADER_VISIBILITY_ALL;
		switch(layout.entries[i].type) {
		case PipelineInputGroupType::ResourceList: {
			parameters[i].ParameterType = D3D12_ROOT_PARAMETER_TYPE_DESCRIPTOR_TABLE;
			parameters[i].DescriptorTable.NumDescriptorRanges = entry.resource_lists.num_resource_lists;

			for(std::size_t range_index = 0; range_index < entry.resource_lists.num_resource_lists; ++range_index) {
				const ResourceListDesc& range = layout.entries[i].resource_lists.resource_lists[range_index];
				ranges[i][range_index].RangeType = d3d12_descriptor_range_type(range.type);
				ranges[i][range_index].NumDescriptors = range.num_resources;
				ranges[i][range_index].BaseShaderRegister = range.binding_slot;
				ranges[i][range_index].RegisterSpace = range.binding_space;
				ranges[i][range_index].OffsetInDescriptorsFromTableStart = range.offset;
				ranges[i][range_index].Flags = D3D12_DESCRIPTOR_RANGE_FLAG_DESCRIPTORS_VOLATILE;
			}
			parameters[i].DescriptorTable.pDescriptorRanges = ranges[i];
			break;
		}
		case PipelineInputGroupType::Buffer: {
			const PipelineInputBufferDesc& buffer = layout.entries[i].buffer;

			parameters[i].ParameterType = d3d12_root_buffer_type(buffer.type);
			parameters[i].Descriptor.ShaderRegister = buffer.binding_slot;
			parameters[i].Descriptor.RegisterSpace = buffer.binding_space;
			break;
		}
		default:
			CD_FAIL("unhandled parameter type");
			break;
		}
	}

	desc.Desc_1_1.NumParameters = layout.num_entries;
	desc.Desc_1_1.pParameters = parameters;
	D3D12_STATIC_SAMPLER_DESC static_samplers[max_pipeline_layout_samplers] {};
	for(std::size_t i = 0; i < layout.num_samplers; ++i) {
		static_samplers[i] = d3d12_static_sampler(layout.samplers[i], layout.sampler_binding_slots[i], layout.sampler_binding_spaces[i]);
	}

	desc.Desc_1_1.NumStaticSamplers = layout.num_samplers;
	desc.Desc_1_1.pStaticSamplers = static_samplers;

	ID3DBlob* blob = nullptr;
	ID3DBlob* error = nullptr;
	D3D12SerializeVersionedRootSignature(&desc, &blob, &error);
	if(error) {
		CD_FAIL(static_cast<const char*>(error->GetBufferPointer()));
	}

	ID3D12RootSignature* root_signature = nullptr;
	HR_ASSERT(adapter.device->CreateRootSignature(1 << adapter.node_index, blob->GetBufferPointer(), blob->GetBufferSize(), IID_PPV_ARGS(&root_signature)));
	blob->Release();

	return root_signature;
}

}