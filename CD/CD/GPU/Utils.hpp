#pragma once

#include <CD/GPU/Common.hpp>

namespace CD::GPU {

constexpr SamplerDesc sampler_defaults(Filter filter, AddressMode address_mode) {
	return {
		false, 0,
		filter, filter, filter,
		address_mode, address_mode, address_mode
	};
}

constexpr SamplerDesc anisotropic_sampler_defaults(std::uint8_t anisotropy) {
	return {
		true, anisotropy
	};
}

constexpr RasterizerParameters rasterizer_defaults(FillMode fill = FillMode::Solid, CullMode cull = CullMode::None, FrontFace front_face = FrontFace::Clockwise) {
	return {
		fill,
		cull,
		front_face,
		true
	};
}

constexpr InputElement input_element_defaults(const char* name, BufferFormat format, std::uint16_t slot, std::uint16_t offset, std::uint16_t index = 0, bool instance_element = false, std::uint16_t instance_data_rate = 0) {
	return {
		name,
		index,
		format,
		slot,
		offset,
		instance_data_rate,
		instance_element
	};
}

constexpr PipelineInputGroup pipeline_input_buffer_defaults(DescriptorType type, std::uint32_t slot, std::uint32_t space) {
	PipelineInputGroup buffer {};
	buffer.type = PipelineInputGroupType::Buffer;
	buffer.buffer = {type, slot, space};
	return buffer;
}

constexpr ResourceListDesc resource_list_defaults(std::uint32_t num_descriptors, DescriptorType type, std::uint32_t slot, std::uint32_t space, std::uint32_t offset = 0) {
	return {
		type,
		slot,
		space,
		num_descriptors,
		offset
	};
}

constexpr PipelineInputGroup pipeline_input_list_defaults(const ResourceListDesc& desc) {
	PipelineInputGroup list {};
	list.type = PipelineInputGroupType::ResourceList;
	list.resource_lists.resource_lists[0] = {desc};
	list.resource_lists.num_resource_lists = 1;
	return list;
}

constexpr DepthStencilTargetDesc depth_stencil_defaults(bool depth_enable = false, bool stencil_enable = false) {
	return {
		depth_enable,
		DepthWriteMask::All,
		CompareOp::LessEqual,
		stencil_enable
	};
}

constexpr GraphicsPipelineDesc graphics_pipeline_defaults(std::uint32_t render_target_count, const DepthStencilTargetDesc& depth_stencil = depth_stencil_defaults(), const RasterizerParameters& rasterizer = rasterizer_defaults(), PrimitiveTopology topology = PrimitiveTopology::TriangleList) {
	GraphicsPipelineDesc pipeline_desc {};
	pipeline_desc.rasterizer = rasterizer;
	pipeline_desc.depth_stencil = depth_stencil;
	pipeline_desc.sample_count = 1;
	pipeline_desc.sample_mask = ~0;
	pipeline_desc.primitive_topology = topology;
	pipeline_desc.render_target_count = render_target_count;
	return pipeline_desc;
}

constexpr RenderPassDepthStencilTargetDesc render_pass_depth_stencil_defaults(RenderPassBeginOp depth_begin = RenderPassBeginOp::Discard, RenderPassEndOp depth_end = RenderPassEndOp::Preserve, float depth_clear = 0.f) {
	return {
		depth_begin,
		RenderPassBeginOp::Preserve,
		depth_end,
		RenderPassEndOp::Preserve,
		BufferFormat::D32_FLOAT,
		BufferFormat::X24_Typeless_G8_UINT,
		depth_clear,
		depth_clear
	};
}

constexpr TextureDesc texture_desc_defaults(std::uint16_t width, std::uint16_t height, BufferFormat format, TextureDimension dimension = TextureDimension::Texture2D, std::uint16_t mip_levels = 1, std::uint16_t depth = 1, BindFlags flags = BindFlags_ShaderResource) {
	return {
		width,
		height,
		depth,
		mip_levels,
		1,
		format,
		dimension,
		flags
	};
}

constexpr TextureView texture_view_defaults(TextureHandle texture, const TextureDesc& desc, std::uint16_t top_mip = 0, std::uint16_t index = 0, std::uint16_t plane = 0, TextureViewDimension dimension = TextureViewDimension::Texture2D) {
	return {
		texture,
		desc.format,
		dimension,
		desc.mip_levels,
		desc.depth,
		top_mip,
		index,
		plane
	};
}

BufferFormat typed_depth_format(BufferFormat);

bool is_depth_stencil_format(BufferFormat);

std::uint32_t row_size(std::uint32_t size, BufferFormat);

}