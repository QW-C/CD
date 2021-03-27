#include <CD/Graphics/Frame.hpp>

namespace CD {

GPUBufferAllocator::GPUBufferAllocator(GPU::Device& device) :
	device(device),
	frame(),
	frame_fence(),
	copy_fence() {

	GPU::BufferDesc upload_buffer_desc {
		upload_buffer_size,
		GPU::BufferStorage::Upload,
		GPU::BindFlags_ShaderResource
	};
	upload_buffer = device.create_buffer(upload_buffer_desc);
	device.map_buffer(upload_buffer, &mapped_memory, 0, upload_buffer_size);

	GPU::BufferDesc gpu_buffer_desc = {
		upload_buffer_size,
		GPU::BufferStorage::Device,
		static_cast<GPU::BindFlags>(GPU::BindFlags_ShaderResource | GPU::BindFlags_RW)
	};
	gpu_buffer = device.create_buffer(gpu_buffer_desc);
}

GPUBufferAllocator::~GPUBufferAllocator() {
	device.unmap_buffer(upload_buffer, 0, upload_buffer_size);
	device.destroy_buffer(upload_buffer);
	device.destroy_buffer(gpu_buffer);
}

BufferAllocation GPUBufferAllocator::create_buffer(std::uint32_t buffer_size, const void* data, bool upload) {
	std::uint32_t offset = frame.head;

	if(std::uint32_t head = static_cast<std::uint32_t>(align(frame.head + buffer_size, buffer_alignment)); head <= upload_buffer_size) {
		frame.head = head;
	}
	else {
		CD_ASSERT(buffer_size < upload_buffer_size);
		offset = 0;
		frame.head = buffer_size;
	}

	if(data) {
		std::memcpy(static_cast<std::uint8_t*>(mapped_memory) + offset, data, buffer_size);
	}

	GPU::BufferHandle buffer_handle = upload_buffer;

	if(upload) {
		buffer_handle = gpu_buffer;

		if(uploads.size() && uploads.front().head == offset) {
			uploads.front().head = frame.head;
		}
		else {
			uploads.push_back({offset, frame.head});
		}
	}
	else {
		CD_ASSERT(data && buffer_size);
	}

	return {buffer_handle, offset};
}

void GPUBufferAllocator::lock(const GPU::Signal& frame_fence) {
	CD_ASSERT(uploads.empty());

	cache.push({frame_fence, frame});
	if(frame.head < frame.tail) {
		frame.tail = 0;
		cache.push({frame_fence, frame});

	}
	else {
		frame.tail = frame.head;
	}
}

void GPUBufferAllocator::reset(const GPU::Signal& completed_fence) {
	while(!cache.empty() && cache.front().first.value <= completed_fence.value) {
		cache.pop();
	}
}

void GPUBufferAllocator::update_data() {
	for(const BufferFrameData& upload_command : uploads) {
		GPU::CopyBufferDesc copy;
		copy.dst = gpu_buffer;
		copy.dst_offset = upload_command.tail;
		copy.src = upload_buffer;
		copy.src_offset = upload_command.tail;
		copy.num_bytes = upload_command.head - upload_command.tail;
		command_buffer.add_command(copy);
	}

	uploads.clear();
}

const GPU::Signal& GPUBufferAllocator::flush() {
	if(command_buffer.get_command_count()) {
		copy_fence = device.submit_commands(command_buffer, GPU::CommandQueueType_Copy);
		command_buffer.reset();
	}

	return copy_fence;
}

const GPU::Signal& GPUBufferAllocator::get_copy_fence() const {
	return copy_fence;
}

CopyContext::CopyContext(GPU::Device& device) :
	device(device),
	curr_offset(),
	copy_fence(),
	completed() {
	GPU::BufferDesc upload_buffer_desc {
		upload_buffer_size,
		GPU::BufferStorage::Upload,
		GPU::BindFlags_None
	};

	upload_buffer_handle = device.create_buffer(upload_buffer_desc);

	device.map_buffer(upload_buffer_handle, &mapped_buffer, 0, upload_buffer_size);
}

CopyContext::~CopyContext() {
	device.unmap_buffer(upload_buffer_handle, 0, upload_buffer_size);
	device.destroy_buffer(upload_buffer_handle);
}

void CopyContext::upload_texture_slice(const Texture* texture, const GPU::TextureView& view, const void* data, std::uint16_t width, std::uint16_t height, std::uint32_t row_pitch) {
	std::uint32_t row = static_cast<std::uint32_t>(align(GPU::row_size(width, view.format), texture_row_alignment));
	CopyBufferAllocation allocation = reserve(row * height, texture_slice_alignment);

	std::uint8_t* ptr = static_cast<std::uint8_t*>(mapped_buffer) + allocation.offset;
	for(std::size_t i = 0; i < height; ++i) {
		std::memcpy(ptr + i * row, static_cast<const std::uint8_t*>(data) + i * row_pitch, row_pitch);
	}

	GPU::CopyBufferToTextureDesc copy {};
	copy.buffer = upload_buffer_handle;
	copy.buffer_offset = allocation.offset;
	copy.width = width;
	copy.height = height;
	copy.row_size = row;
	copy.texture = view;

	command_buffer.add_command(copy);
}

void CopyContext::upload_buffer(const GPU::BufferView& buffer, const void* data) {
	CopyBufferAllocation allocation = reserve(buffer.size);

	std::memcpy(static_cast<std::uint8_t*>(mapped_buffer) + allocation.offset, data, buffer.size);

	GPU::CopyBufferDesc copy {};
	copy.dst = buffer.buffer;
	copy.dst_offset = buffer.offset;
	copy.num_bytes = buffer.size;
	copy.src = upload_buffer_handle;
	copy.src_offset = allocation.offset;

	command_buffer.add_command(copy);
}

void CopyContext::flush() {
	copy_fence = device.submit_commands(command_buffer, GPU::CommandQueueType_Copy);
	device.wait_for_fence(copy_fence);
	command_buffer.reset();

	free_list.insert(free_list.end(), cache.begin(), cache.end());

	cache.clear();
}

CopyContext::CopyBufferAllocation CopyContext::reserve(std::uint64_t num_bytes, std::uint64_t alignment) {
	for(auto it = free_list.begin(); it != free_list.end(); ++it) {
		if(num_bytes <= it->size && it->offset % alignment == 0) {
			CopyBufferAllocation& ret = cache.emplace_back(*it);
			free_list.erase(it);
			return ret;
		}
	}

	curr_offset = static_cast<std::uint32_t>(align(curr_offset, alignment));
	CopyBufferAllocation allocation {
		curr_offset,
		static_cast<std::uint32_t>(num_bytes)
	};
	curr_offset += static_cast<std::uint32_t>(num_bytes);

	CD_ASSERT(std::uint32_t(allocation.offset + allocation.size) < upload_buffer_size);
	cache.push_back(allocation);
	return allocation;
}

Frame::Frame(GPU::Device& device, float width, float height) :
	device(device),
	viewport(),
	present_fences(),
	completed_fence(),
	present_index(),
	buffer_allocator(device),
	copy_context(device) {

	viewport.width = width;
	viewport.height = height;
	viewport.min_z = 0;
	viewport.max_z = 1.f;
}

Frame::~Frame() {
	for(auto& render_pass : render_passes) {
		device.destroy_pipeline_resource(render_pass->handle);
	}

	for(auto& pipeline : graphics_pipelines) {
		device.destroy_pipeline_resource(pipeline->handle);
	}

	for(auto& pipeline : compute_pipelines) {
		device.destroy_pipeline_resource(pipeline->handle);
	}

	destroy_textures();
}

FrameResourceIndex Frame::add_texture(GPU::TextureDesc& desc, bool create_views_flag) {
	auto& texture = texture_pool.emplace_back(std::make_unique<FrameTexture>());
	texture->texture.handle = GPU::TextureHandle::Invalid;
	texture->texture.desc = desc;
	texture->state = GPU::ResourceState::Common;

	if(create_views_flag) {
		texture->views = views.emplace_back(std::make_unique<FrameTextureViews>()).get();
		texture->views->srv = device.create_pipeline_input_list(1);
		texture->views->uav = device.create_pipeline_input_list(1);
	}

	return static_cast<FrameResourceIndex>(texture_pool.size() - 1);
}

FrameTexture& Frame::get_texture(FrameResourceIndex handle) {
	FrameTexture& texture = *texture_pool[handle];

	if(GPU::TextureHandle& texture_handle = texture.texture.handle; texture_handle == GPU::TextureHandle::Invalid) {
		texture_handle = device.create_texture(texture.texture.desc);

		if(texture.views) {
			create_views(texture);
		}
	}

	return texture;
}

const RenderPass* Frame::create_render_pass(const GPU::RenderPassDesc& desc) {
	auto& render_pass = render_passes.emplace_back(std::make_unique<RenderPass>());

	render_pass->handle = device.create_render_pass(desc);
	render_pass->desc = desc;
	return render_pass.get();
}

const GraphicsPipeline* Frame::create_pipeline(const GPU::GraphicsPipelineDesc& desc, const GPU::PipelineInputLayout& layout) {
	auto& pipeline = graphics_pipelines.emplace_back(std::make_unique<GraphicsPipeline>());

	pipeline->handle = device.create_pipeline_state(desc, layout);
	pipeline->desc = desc;
	pipeline->layout = layout;

	return pipeline.get();
}

const ComputePipeline* Frame::create_pipeline(const GPU::ComputePipelineDesc& desc, const GPU::PipelineInputLayout& layout) {
	auto& pipeline = compute_pipelines.emplace_back(std::make_unique<ComputePipeline>());

	pipeline->handle = device.create_pipeline_state(desc, layout);
	pipeline->desc = desc;
	pipeline->layout = layout;

	return pipeline.get();
}

void Frame::bind_resources(const FrameResourceIndex* textures, std::size_t num_textures, GPU::ResourceState target_state) {
	for(std::uint32_t i = 0; i < num_textures; ++i) {
		auto& texture = get_texture(textures[i]);

		if(texture.state != target_state) {
			GPU::LayoutBarrierDesc barrier {};
			barrier.texture = GPU::texture_view_defaults(texture.texture.handle, texture.texture.desc);
			barrier.all_subresources = true;
			barrier.before = texture.state;
			barrier.after = target_state;

			command_buffer.add_command(barrier);

			texture.state = target_state;
		}
	}
}

void Frame::begin() {
	if(const GPU::Signal& present = present_fences[(present_index + 1) % max_latency]; completed_fence.value + max_latency < present.value) {
		device.wait_for_fence(present);
		completed_fence = present;
	}

	buffer_allocator.reset(completed_fence);
}

void Frame::present() {
	device.wait(buffer_allocator.flush(), GPU::CommandQueueType_Direct);
	device.submit_commands(command_buffer, GPU::CommandQueueType_Direct);

	present_fences[present_index] = device.reset();

	buffer_allocator.lock(present_fences[present_index]);

	command_buffer.reset();

	present_index = (present_index + 1) % max_latency;
}

void Frame::wait() {
	device.signal(GPU::CommandQueueType_Direct);
	device.wait_for_fence(present_fences[(present_index + 1) % max_latency]);
}

bool Frame::resize_buffers(float width, float height) {
	bool changed = width >= viewport.width || height >= viewport.height;
	if(changed) {
		completed_fence = device.submit_commands(command_buffer, GPU::CommandQueueType_Direct);
		device.signal(GPU::CommandQueueType_Direct);
		device.wait_for_fence(completed_fence);

		command_buffer.reset();

		destroy_textures();
		device.resize_buffers(static_cast<std::uint32_t>(width), static_cast<std::uint32_t>(height));
	}
	viewport.width = width;
	viewport.height = height;
	return changed;
}

const GPU::Viewport& Frame::get_viewport() const {
	return viewport;
}

GPU::ShaderCompiler& Frame::get_shader_compiler() {
	return device.get_shader_compiler();
}

GPU::Device& Frame::get_device() {
	return device;
}

GPU::CommandBuffer& Frame::get_command_buffer() {
	return command_buffer;
};

GPUBufferAllocator& Frame::get_buffer_allocator() {
	return buffer_allocator;
}

CopyContext& Frame::get_copy_context() {
	return copy_context;
}

void Frame::create_views(FrameTexture& texture) {
	GPU::TextureView view = GPU::texture_view_defaults(texture.texture.handle, texture.texture.desc);

	CD_ASSERT(texture.views);

	device.update_pipeline_input_list(texture.views->srv, GPU::DescriptorType::SRV, &view, 1, 0);

	if(texture.texture.desc.flags & GPU::BindFlags_RW) {
		device.update_pipeline_input_list(texture.views->uav, GPU::DescriptorType::UAV, &view, 1, 0);
	}
}

void Frame::destroy_textures() {
	for(auto& texture : texture_pool) {
		if(GPU::TextureHandle& handle = texture->texture.handle; handle != GPU::TextureHandle::Invalid) {
			device.destroy_texture(handle);
			handle = GPU::TextureHandle::Invalid;
		}
	}
	for(auto& view : views) {
		if(auto& srv = view->srv; srv.handle) {
			device.destroy_pipeline_resource(srv);
			srv = {};
		}
		if(auto& uav = view->uav; uav.handle) {
			device.destroy_pipeline_resource(uav);
			uav = {};
		}
	}
}

}