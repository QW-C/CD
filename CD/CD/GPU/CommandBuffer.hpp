#pragma once

#include <CD/GPU/Common.hpp>
#include <CD/Common/Debug.hpp>
#include <memory>

namespace CD::GPU {

enum class CommandType : std::uint8_t {
	Draw,
	LayoutBarrier,
	ResourceBarrier,
	Dispatch,
	DispatchIndirect,
	BeginRenderPass,
	EndRenderPass,
	CopyBuffer,
	CopyBufferToTexture,
	CopyTexture,
	CopyTextureToBuffer,
	InsertTimestamp,
	ResolveTimestamps,
	CopyToSwapChain,
	Invalid
};

struct CommandBase {
	CommandBase(CommandType type, std::uint32_t size) :
		type(type),
		size(size) {
	}

	CommandType type = CommandType::Invalid;
	std::uint32_t size;
};

template<CommandType Type>
struct CommandTyped : CommandBase {
	CommandTyped() : CommandBase(Type, 0) {}
};

struct DrawDesc : CommandTyped<CommandType::Draw> {
	PipelineHandle graphics_pipeline;
	PipelineInputState pipeline_input_state;
	Scissor rect;
	BufferHandle input_buffer;
	BufferHandle index_buffer;
	std::uint16_t num_instances;
	std::uint8_t num_vertex_buffers;
	std::uint32_t num_elements;
	std::uint32_t vertex_buffer_offsets[max_vertex_buffers];
	std::uint32_t vertex_buffer_sizes[max_vertex_buffers];
	std::uint32_t vertex_buffer_strides[max_vertex_buffers];
	std::uint32_t index_buffer_size;
	std::uint32_t index_buffer_offset;
};

struct LayoutBarrierDesc : CommandTyped<CommandType::LayoutBarrier> {
	TextureView texture;
	ResourceState before;
	ResourceState after;
	bool all_subresources;
};

struct ResourceBarrierDesc : CommandTyped<CommandType::ResourceBarrier> {
	BufferHandle buffers[max_resource_barriers];
	TextureHandle textures[max_resource_barriers];
	std::uint32_t num_buffer_barriers;
	std::uint32_t num_texture_barriers;
};

struct DispatchDesc : CommandTyped<CommandType::Dispatch> {
	PipelineHandle compute_pipeline;
	PipelineInputState pipeline_input_state;
	std::uint32_t x;
	std::uint32_t y;
	std::uint32_t z;
};

struct DispatchIndirectBuffer {
	std::uint32_t x;
	std::uint32_t y;
	std::uint32_t z;
};

struct DispatchIndirectDesc : CommandTyped<CommandType::DispatchIndirect> {
	PipelineHandle compute_pipeline;
	PipelineInputState pipeline_input_state;
	BufferHandle args;
	std::uint32_t offset;
};

struct BeginRenderPassDesc : CommandTyped<CommandType::BeginRenderPass> {
	TextureView color[max_render_targets];
	std::uint32_t render_target_count;
	TextureView depth_stencil_target;
	PipelineHandle render_pass;
	PrimitiveTopology primitive_topology;
	Viewport viewport;
	bool depth_write;
	bool stencil_write;
};

struct EndRenderPassDesc : CommandTyped<CommandType::EndRenderPass> {
};

struct CopyBufferDesc : CommandTyped<CommandType::CopyBuffer> {
	BufferHandle dst;
	std::uint64_t dst_offset;
	BufferHandle src;
	std::uint64_t src_offset;
	std::uint64_t num_bytes;
};

struct CopyBufferToTextureDesc : CommandTyped<CommandType::CopyBufferToTexture> {
	TextureView texture;
	std::uint32_t width;
	std::uint32_t height;
	BufferHandle buffer;
	std::uint32_t buffer_offset;
	std::uint32_t row_size;
};

struct CopyTextureDesc : CommandTyped<CommandType::CopyTexture> {
	TextureView dst;
	TextureView src;
};

struct CopyTextureToBufferDesc : CommandTyped<CommandType::CopyTextureToBuffer> {
	TextureView texture;
	std::uint32_t width;
	std::uint32_t height;
	BufferHandle buffer;
	std::uint32_t buffer_offset;
	std::uint32_t row_size;
};

struct InsertTimestampDesc : CommandTyped<CommandType::InsertTimestamp> {
	std::uint32_t index;
};

struct ResolveTimestampsDesc : CommandTyped<CommandType::ResolveTimestamps> {
	std::uint32_t index;
	std::uint32_t timestamp_count;
	BufferHandle dest;
	std::uint32_t aligned_offset;
};

struct CopyToSwapChainDesc : CommandTyped<CommandType::CopyToSwapChain> {
	TextureView texture;
	ResourceState texture_state;
	//bool resolve;
};

class CommandBuffer {
public:
	CommandBuffer(std::size_t size = default_command_buffer_size);

	template<typename CommandDesc> void add_command(CommandDesc&);
	void reset();

	const std::uint8_t* get_commands() const;
	const std::uint8_t* end() const;
	CommandType get_command_type(const void* command) const;
	std::uint32_t get_command_size(const void* command) const;
	std::uint32_t get_command_count() const;
private:
	static constexpr std::size_t default_command_buffer_size = 1 << 20;

	std::unique_ptr<std::uint8_t[]> commands;
	std::size_t size;
	std::size_t offset;

	std::uint32_t command_counter;
};

inline CommandBuffer::CommandBuffer(std::size_t size) :
	commands(std::make_unique<std::uint8_t[]>(size)),
	size(size),
	offset(),
	command_counter() {
}

inline void CommandBuffer::reset() {
	offset = 0;
	command_counter = 0;
}

template<typename CommandDesc>
inline void CommandBuffer::add_command(CommandDesc& desc) {
	static_assert(std::is_trivially_destructible<CommandDesc>::value);

	desc.size = sizeof(desc);
	std::size_t ptr = align(offset + sizeof(CommandDesc), alignof(CommandDesc));
	CD_ASSERT(ptr < size);
	std::memcpy(&commands[offset], &desc, sizeof(CommandDesc));
	offset = ptr;
	++command_counter;
}

inline const std::uint8_t* CommandBuffer::get_commands() const {
	return &commands[0];
}

inline const std::uint8_t* CommandBuffer::end() const {
	return &commands[offset];
}

inline CommandType CommandBuffer::get_command_type(const void* command) const {
	return static_cast<const CommandBase*>(command)->type;
}

inline std::uint32_t CommandBuffer::get_command_size(const void* command) const {
	return static_cast<const CommandBase*>(command)->size;
}

inline std::uint32_t CommandBuffer::get_command_count() const {
	return command_counter;
}

}