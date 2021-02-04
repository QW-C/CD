#pragma once

#include <CD/Common/Common.hpp>
#include <CD/Common/Debug.hpp>
#include <CD/Common/ResourcePool.hpp>
#include <CD/GPU/Common.hpp>
#include <d3d12.h>
#include <dxgi1_6.h>
#include <vector>

namespace CD::GPU::D3D12 {

constexpr std::uint32_t d3d12_shader_descriptor_heap_size = 1'000'000;
constexpr std::uint32_t cpu_descriptor_count = 1024;

using GPUVA = D3D12_GPU_VIRTUAL_ADDRESS;
using CPUHandle = D3D12_CPU_DESCRIPTOR_HANDLE;
using GPUHandle = D3D12_GPU_DESCRIPTOR_HANDLE;

#define HR_ASSERT(result) CD_ASSERT(SUCCEEDED(result));

constexpr DXGI_FORMAT dxgi_format_table[] {
	DXGI_FORMAT_UNKNOWN,
	DXGI_FORMAT_R32G32B32A32_TYPELESS,
	DXGI_FORMAT_R32G32B32A32_FLOAT,
	DXGI_FORMAT_R32G32B32A32_UINT,
	DXGI_FORMAT_R32G32B32A32_SINT,
	DXGI_FORMAT_R32G32B32_TYPELESS,
	DXGI_FORMAT_R32G32B32_FLOAT,
	DXGI_FORMAT_R32G32B32_UINT,
	DXGI_FORMAT_R32G32B32_SINT,
	DXGI_FORMAT_R16G16B16A16_TYPELESS,
	DXGI_FORMAT_R16G16B16A16_FLOAT,
	DXGI_FORMAT_R16G16B16A16_UNORM,
	DXGI_FORMAT_R16G16B16A16_UINT,
	DXGI_FORMAT_R16G16B16A16_SNORM,
	DXGI_FORMAT_R16G16B16A16_SINT,
	DXGI_FORMAT_R32G32_TYPELESS,
	DXGI_FORMAT_R32G32_FLOAT,
	DXGI_FORMAT_R32G32_UINT,
	DXGI_FORMAT_R32G32_SINT,
	DXGI_FORMAT_R32G8X24_TYPELESS,
	DXGI_FORMAT_D32_FLOAT_S8X24_UINT,
	DXGI_FORMAT_R32_FLOAT_X8X24_TYPELESS,
	DXGI_FORMAT_X32_TYPELESS_G8X24_UINT,
	DXGI_FORMAT_R10G10B10A2_TYPELESS,
	DXGI_FORMAT_R10G10B10A2_UNORM,
	DXGI_FORMAT_R10G10B10A2_UINT,
	DXGI_FORMAT_R11G11B10_FLOAT,
	DXGI_FORMAT_R8G8B8A8_TYPELESS,
	DXGI_FORMAT_R8G8B8A8_UNORM,
	DXGI_FORMAT_R8G8B8A8_UNORM_SRGB,
	DXGI_FORMAT_R8G8B8A8_UINT,
	DXGI_FORMAT_R8G8B8A8_SNORM,
	DXGI_FORMAT_R8G8B8A8_SINT,
	DXGI_FORMAT_R16G16_TYPELESS,
	DXGI_FORMAT_R16G16_FLOAT,
	DXGI_FORMAT_R16G16_UNORM,
	DXGI_FORMAT_R16G16_UINT,
	DXGI_FORMAT_R16G16_SNORM,
	DXGI_FORMAT_R16G16_SINT,
	DXGI_FORMAT_R32_TYPELESS,
	DXGI_FORMAT_D32_FLOAT,
	DXGI_FORMAT_R32_FLOAT,
	DXGI_FORMAT_R32_UINT,
	DXGI_FORMAT_R32_SINT,
	DXGI_FORMAT_R24G8_TYPELESS,
	DXGI_FORMAT_D24_UNORM_S8_UINT,
	DXGI_FORMAT_R24_UNORM_X8_TYPELESS,
	DXGI_FORMAT_X24_TYPELESS_G8_UINT,
	DXGI_FORMAT_R8G8_TYPELESS,
	DXGI_FORMAT_R8G8_UNORM,
	DXGI_FORMAT_R8G8_UINT,
	DXGI_FORMAT_R8G8_SNORM,
	DXGI_FORMAT_R8G8_SINT,
	DXGI_FORMAT_R16_TYPELESS,
	DXGI_FORMAT_R16_FLOAT,
	DXGI_FORMAT_D16_UNORM,
	DXGI_FORMAT_R16_UNORM,
	DXGI_FORMAT_R16_UINT,
	DXGI_FORMAT_R16_SNORM,
	DXGI_FORMAT_R16_SINT,
	DXGI_FORMAT_R8_TYPELESS,
	DXGI_FORMAT_R8_UNORM,
	DXGI_FORMAT_R8_UINT,
	DXGI_FORMAT_R8_SNORM,
	DXGI_FORMAT_R8_SINT,
	DXGI_FORMAT_A8_UNORM,
	DXGI_FORMAT_R1_UNORM,
	DXGI_FORMAT_R8G8_B8G8_UNORM,
	DXGI_FORMAT_G8R8_G8B8_UNORM,
	DXGI_FORMAT_BC1_TYPELESS,
	DXGI_FORMAT_BC1_UNORM,
	DXGI_FORMAT_BC1_UNORM_SRGB,
	DXGI_FORMAT_BC2_TYPELESS,
	DXGI_FORMAT_BC2_UNORM,
	DXGI_FORMAT_BC2_UNORM_SRGB,
	DXGI_FORMAT_BC3_TYPELESS,
	DXGI_FORMAT_BC3_UNORM,
	DXGI_FORMAT_BC3_UNORM_SRGB,
	DXGI_FORMAT_BC4_TYPELESS,
	DXGI_FORMAT_BC4_UNORM,
	DXGI_FORMAT_BC4_SNORM,
	DXGI_FORMAT_BC5_TYPELESS,
	DXGI_FORMAT_BC5_UNORM,
	DXGI_FORMAT_BC5_SNORM,
	DXGI_FORMAT_B5G6R5_UNORM,
	DXGI_FORMAT_B5G5R5A1_UNORM,
	DXGI_FORMAT_B8G8R8A8_UNORM,
	DXGI_FORMAT_B8G8R8X8_UNORM,
	DXGI_FORMAT_B8G8R8A8_TYPELESS,
	DXGI_FORMAT_B8G8R8A8_UNORM_SRGB,
	DXGI_FORMAT_B8G8R8X8_TYPELESS,
	DXGI_FORMAT_B8G8R8X8_UNORM_SRGB,
	DXGI_FORMAT_BC6H_TYPELESS,
	DXGI_FORMAT_BC6H_UF16,
	DXGI_FORMAT_BC6H_SF16,
	DXGI_FORMAT_BC7_TYPELESS,
	DXGI_FORMAT_BC7_UNORM,
	DXGI_FORMAT_BC7_UNORM_SRGB,
	DXGI_FORMAT_B4G4R4A4_UNORM
};

constexpr D3D12_PRIMITIVE_TOPOLOGY d3d12_primitive_topology[] {
	D3D_PRIMITIVE_TOPOLOGY_POINTLIST,
	D3D_PRIMITIVE_TOPOLOGY_LINELIST,
	D3D_PRIMITIVE_TOPOLOGY_LINESTRIP,
	D3D_PRIMITIVE_TOPOLOGY_TRIANGLELIST,
	D3D_PRIMITIVE_TOPOLOGY_TRIANGLESTRIP,
	D3D_PRIMITIVE_TOPOLOGY_LINELIST_ADJ,
	D3D_PRIMITIVE_TOPOLOGY_LINESTRIP_ADJ,
	D3D_PRIMITIVE_TOPOLOGY_TRIANGLELIST_ADJ,
	D3D_PRIMITIVE_TOPOLOGY_TRIANGLESTRIP_ADJ
};

constexpr D3D12_BLEND_OP d3d12_blend_op[] {
	D3D12_BLEND_OP_ADD,
	D3D12_BLEND_OP_SUBTRACT,
	D3D12_BLEND_OP_REV_SUBTRACT,
	D3D12_BLEND_OP_MIN,
	D3D12_BLEND_OP_MAX
};

constexpr D3D12_BLEND d3d12_blend[] {
	D3D12_BLEND_ZERO,
	D3D12_BLEND_ONE,
	D3D12_BLEND_SRC_COLOR,
	D3D12_BLEND_INV_SRC_COLOR,
	D3D12_BLEND_SRC_ALPHA,
	D3D12_BLEND_INV_SRC_ALPHA,
	D3D12_BLEND_DEST_ALPHA,
	D3D12_BLEND_INV_DEST_ALPHA,
	D3D12_BLEND_DEST_COLOR,
	D3D12_BLEND_INV_DEST_COLOR,
	D3D12_BLEND_SRC_ALPHA_SAT,
	D3D12_BLEND_BLEND_FACTOR,
	D3D12_BLEND_INV_BLEND_FACTOR,
	D3D12_BLEND_SRC1_COLOR,
	D3D12_BLEND_INV_SRC1_COLOR,
	D3D12_BLEND_SRC1_ALPHA,
	D3D12_BLEND_INV_SRC1_ALPHA
};

constexpr D3D12_TEXTURE_ADDRESS_MODE d3d12_address_mode[] {
	D3D12_TEXTURE_ADDRESS_MODE_WRAP,
	D3D12_TEXTURE_ADDRESS_MODE_MIRROR,
	D3D12_TEXTURE_ADDRESS_MODE_CLAMP,
	D3D12_TEXTURE_ADDRESS_MODE_MIRROR_ONCE
};

constexpr D3D12_FILTER_TYPE d3d12_filter[] {
	D3D12_FILTER_TYPE_POINT,
	D3D12_FILTER_TYPE_LINEAR
};

constexpr D3D12_LOGIC_OP d3d12_logic_op[] {
	D3D12_LOGIC_OP_CLEAR,
	D3D12_LOGIC_OP_SET,
	D3D12_LOGIC_OP_COPY,
	D3D12_LOGIC_OP_COPY_INVERTED,
	D3D12_LOGIC_OP_NOOP,
	D3D12_LOGIC_OP_INVERT,
	D3D12_LOGIC_OP_AND,
	D3D12_LOGIC_OP_NAND,
	D3D12_LOGIC_OP_OR,
	D3D12_LOGIC_OP_NOR,
	D3D12_LOGIC_OP_XOR,
	D3D12_LOGIC_OP_EQUIV,
	D3D12_LOGIC_OP_AND_REVERSE,
	D3D12_LOGIC_OP_AND_INVERTED,
	D3D12_LOGIC_OP_OR_REVERSE,
	D3D12_LOGIC_OP_OR_INVERTED
};

constexpr D3D12_COMPARISON_FUNC d3d12_comparison_func[] {
	D3D12_COMPARISON_FUNC_NEVER,
	D3D12_COMPARISON_FUNC_LESS,
	D3D12_COMPARISON_FUNC_EQUAL,
	D3D12_COMPARISON_FUNC_LESS_EQUAL,
	D3D12_COMPARISON_FUNC_GREATER,
	D3D12_COMPARISON_FUNC_NOT_EQUAL,
	D3D12_COMPARISON_FUNC_GREATER_EQUAL,
	D3D12_COMPARISON_FUNC_ALWAYS
};

constexpr D3D12_STENCIL_OP d3d12_stencil_op[] {
	D3D12_STENCIL_OP_KEEP,
	D3D12_STENCIL_OP_ZERO,
	D3D12_STENCIL_OP_REPLACE,
	D3D12_STENCIL_OP_INCR_SAT,
	D3D12_STENCIL_OP_DECR_SAT,
	D3D12_STENCIL_OP_INVERT,
	D3D12_STENCIL_OP_INCR,
	D3D12_STENCIL_OP_DECR,
};

constexpr DXGI_FORMAT dxgi_format(BufferFormat format) {
	CD_ASSERT(format < GPU::BufferFormat::Unused);
	return dxgi_format_table[static_cast<std::size_t>(format)];
}

constexpr D3D12_RESOURCE_DIMENSION d3d12_resource_dimension(TextureDimension dimension) {
	CD_ASSERT(dimension <= TextureDimension::Texture3D);
	switch(dimension) {
	case TextureDimension::Texture1D:
		return D3D12_RESOURCE_DIMENSION_TEXTURE1D;
	case TextureDimension::Texture2D:
		return D3D12_RESOURCE_DIMENSION_TEXTURE2D;
	case TextureDimension::Texture3D:
		return D3D12_RESOURCE_DIMENSION_TEXTURE3D;
	default:
		CD_FAIL("unhandled value");
	}
	return D3D12_RESOURCE_DIMENSION_BUFFER;
}

constexpr D3D12_DEPTH_STENCILOP_DESC d3d12_depth_stencil_op(const DepthStencilOp& desc) {
	return {
		d3d12_stencil_op[static_cast<std::size_t>(desc.stencil_fail_op)],
		d3d12_stencil_op[static_cast<std::size_t>(desc.stencil_depth_fail_op)],
		d3d12_stencil_op[static_cast<std::size_t>(desc.stencil_pass_op)],
		d3d12_comparison_func[static_cast<std::size_t>(desc.stencil_op)],
	};
}

constexpr D3D12_PRIMITIVE_TOPOLOGY_TYPE d3d12_primitive_topology_type(PrimitiveTopology topology) {
	switch(topology) {
	case PrimitiveTopology::PointList:
		return D3D12_PRIMITIVE_TOPOLOGY_TYPE_POINT;
	case PrimitiveTopology::LineList:
	case PrimitiveTopology::LineStrip:
	case PrimitiveTopology::LineListAdjacency:
	case PrimitiveTopology::LineStripAdjacency:
		return D3D12_PRIMITIVE_TOPOLOGY_TYPE_LINE;
	case PrimitiveTopology::TriangleList:
	case PrimitiveTopology::TriangleStrip:
	case PrimitiveTopology::TriangleListAdjacency:
	case PrimitiveTopology::TriangleStripAdjacency:
		return D3D12_PRIMITIVE_TOPOLOGY_TYPE_TRIANGLE;
	case PrimitiveTopology::PatchList:
		return D3D12_PRIMITIVE_TOPOLOGY_TYPE_PATCH;
	default:
		return D3D12_PRIMITIVE_TOPOLOGY_TYPE_UNDEFINED;
	}
}

constexpr D3D12_CULL_MODE d3d12_cull_mode(CullMode cull) {
	switch(cull) {
	case CullMode::None:
		return D3D12_CULL_MODE_NONE;
	case CullMode::Back:
		return D3D12_CULL_MODE_BACK;
	case CullMode::Front:
		return D3D12_CULL_MODE_FRONT;
	default:
		CD_FAIL("unhandled value");
	}
	return D3D12_CULL_MODE_NONE;
}

constexpr D3D12_FILL_MODE d3d12_fill_mode(FillMode fill) {
	switch(fill) {
	case FillMode::Solid:
		return D3D12_FILL_MODE_SOLID;
	case FillMode::Wireframe:
		return D3D12_FILL_MODE_WIREFRAME;
	default:
		CD_FAIL("unhandled value");
	}
	return D3D12_FILL_MODE_WIREFRAME;
}

constexpr D3D12_BLEND get_d3d12_blend(BlendFactor blend) {
	CD_ASSERT(blend <= BlendFactor::InvSrc1Alpha);
	return d3d12_blend[static_cast<std::size_t>(blend)];
}

constexpr D3D12_BLEND_OP get_d3d12_blend_op(BlendOp op) {
	CD_ASSERT(op <= BlendOp::ReverseSubstract);
	return d3d12_blend_op[static_cast<std::size_t>(op)];
}

constexpr D3D12_RENDER_TARGET_BLEND_DESC d3d12_render_target_blend_desc(const RenderTargetBlend& blend) {
	return {
		blend.blend_enable,
		blend.logic_op_enable,
		get_d3d12_blend(blend.src_blend),
		get_d3d12_blend(blend.dst_blend),
		get_d3d12_blend_op(blend.blend_op),
		get_d3d12_blend(blend.src_alpha_blend),
		get_d3d12_blend(blend.dst_alpha_blend),
		get_d3d12_blend_op(blend.alpha_blend_op),
		d3d12_logic_op[static_cast<std::size_t>(blend.logic_op)],
		blend.render_target_write_mask
	};
}

constexpr D3D12_FILTER_TYPE get_d3d12_filter(Filter filter) {
	CD_ASSERT(filter <= Filter::Linear);
	return d3d12_filter[static_cast<std::size_t>(filter)];
}

constexpr D3D12_TEXTURE_ADDRESS_MODE get_d3d12_address_mode(AddressMode address_mode) {
	CD_ASSERT(address_mode <= AddressMode::MirrorClamp);
	return d3d12_address_mode[static_cast<std::size_t>(address_mode)];
}

constexpr D3D12_DESCRIPTOR_RANGE_TYPE d3d12_descriptor_range_type(DescriptorType type) {
	switch(type) {
	case DescriptorType::CBV:
		return D3D12_DESCRIPTOR_RANGE_TYPE_CBV;
	case DescriptorType::SRV:
		return D3D12_DESCRIPTOR_RANGE_TYPE_SRV;
	case DescriptorType::UAV:
		return D3D12_DESCRIPTOR_RANGE_TYPE_UAV;
	default:
		CD_FAIL("unhandled value");
	}
	return D3D12_DESCRIPTOR_RANGE_TYPE_SRV;
}

constexpr D3D12_ROOT_PARAMETER_TYPE d3d12_root_buffer_type(DescriptorType type) {
	switch(type) {
	case DescriptorType::CBV:
		return D3D12_ROOT_PARAMETER_TYPE_CBV;
	case DescriptorType::SRV:
		return D3D12_ROOT_PARAMETER_TYPE_SRV;
	case DescriptorType::UAV:
		return D3D12_ROOT_PARAMETER_TYPE_UAV;
	default:
		CD_FAIL("unhandled value");
	}
	return D3D12_ROOT_PARAMETER_TYPE_DESCRIPTOR_TABLE;
}

constexpr D3D12_STATIC_SAMPLER_DESC d3d12_static_sampler(const SamplerDesc& desc, std::uint32_t slot, std::uint32_t space) {
	D3D12_FILTER filter {};
	if(desc.anisotropy_enable) {
		filter = D3D12_ENCODE_ANISOTROPIC_FILTER(D3D12_FILTER_REDUCTION_TYPE_STANDARD);
	}
	else {
		filter = D3D12_ENCODE_BASIC_FILTER(get_d3d12_filter(desc.min_filter), get_d3d12_filter(desc.min_filter), get_d3d12_filter(desc.min_filter), D3D12_FILTER_REDUCTION_TYPE_STANDARD);
	}
	return {
		filter,
		get_d3d12_address_mode(desc.address_u),
		get_d3d12_address_mode(desc.address_v),
		get_d3d12_address_mode(desc.address_w),
		desc.mip_lod_bias,
		desc.max_anisotropy,
		d3d12_comparison_func[static_cast<std::size_t>(desc.comparison_func)],
		D3D12_STATIC_BORDER_COLOR_TRANSPARENT_BLACK,
		desc.min_lod,
		desc.max_lod,
		slot,
		space,
		D3D12_SHADER_VISIBILITY_ALL
	};
}

constexpr D3D12_RENDER_PASS_BEGINNING_ACCESS_TYPE d3d12_beginning_access_op(RenderPassBeginOp op) {
	switch(op) {
	case RenderPassBeginOp::Discard:
		return D3D12_RENDER_PASS_BEGINNING_ACCESS_TYPE_DISCARD;
	case RenderPassBeginOp::Clear:
		return D3D12_RENDER_PASS_BEGINNING_ACCESS_TYPE_CLEAR;
	case RenderPassBeginOp::Preserve:
		return D3D12_RENDER_PASS_BEGINNING_ACCESS_TYPE_PRESERVE;
	default:
		CD_FAIL("unhandled value");
	}
	return D3D12_RENDER_PASS_BEGINNING_ACCESS_TYPE_NO_ACCESS;
}

constexpr D3D12_RENDER_PASS_ENDING_ACCESS_TYPE d3d12_ending_access_op(RenderPassEndOp op) {
	switch(op) {
	case RenderPassEndOp::Discard:
		return D3D12_RENDER_PASS_ENDING_ACCESS_TYPE_DISCARD;
	case RenderPassEndOp::Resolve:
		return D3D12_RENDER_PASS_ENDING_ACCESS_TYPE_RESOLVE;
	case RenderPassEndOp::Preserve:
		return D3D12_RENDER_PASS_ENDING_ACCESS_TYPE_PRESERVE;
	default:
		CD_FAIL("unhandled value");
	}
	return D3D12_RENDER_PASS_ENDING_ACCESS_TYPE_NO_ACCESS;
}

constexpr D3D12_RENDER_PASS_BEGINNING_ACCESS d3d12_depth_stencil_beginning_access(RenderPassBeginOp op, BufferFormat clear_format, float depth_clear, std::uint8_t stencil_clear) {
	D3D12_RENDER_PASS_BEGINNING_ACCESS render_pass {};
	render_pass.Type = d3d12_beginning_access_op(op);
	if(render_pass.Type == D3D12_RENDER_PASS_BEGINNING_ACCESS_TYPE_CLEAR) {
		render_pass.Clear.ClearValue.DepthStencil.Depth = depth_clear;
		render_pass.Clear.ClearValue.DepthStencil.Stencil = stencil_clear;
		render_pass.Clear.ClearValue.Format = dxgi_format(clear_format);
	}
	return render_pass;
}

constexpr D3D12_RESOURCE_STATES d3d12_initial_state(BufferStorage storage) {
	switch(storage) {
	case BufferStorage::Upload:
		return D3D12_RESOURCE_STATE_GENERIC_READ;
	case BufferStorage::Device:
		return D3D12_RESOURCE_STATE_COMMON;
	case BufferStorage::Readback:
		return D3D12_RESOURCE_STATE_COPY_DEST;
	default:
		CD_FAIL("unhandled value");
	}
	return D3D12_RESOURCE_STATE_COMMON;
}

constexpr D3D12_HEAP_TYPE d3d12_heap_type(BufferStorage storage) {
	switch(storage) {
	case BufferStorage::Upload:
		return D3D12_HEAP_TYPE_UPLOAD;
	case BufferStorage::Device:
		return D3D12_HEAP_TYPE_DEFAULT;
	case BufferStorage::Readback:
		return D3D12_HEAP_TYPE_READBACK;
	default:
		CD_FAIL("unhandled value");
	}
	return D3D12_HEAP_TYPE_DEFAULT;
}

constexpr D3D12_SHADER_RESOURCE_VIEW_DESC srv_desc_texture(const TextureView& view) {
	D3D12_SHADER_RESOURCE_VIEW_DESC srv {};

	srv.Format = dxgi_format(view.format);
	srv.Shader4ComponentMapping = D3D12_DEFAULT_SHADER_4_COMPONENT_MAPPING;

	switch(view.dimension) {
	case TextureViewDimension::Texture1D:
		srv.ViewDimension = D3D12_SRV_DIMENSION_TEXTURE1D;
		srv.Texture1D.MostDetailedMip = view.mip_level;
		srv.Texture1D.MipLevels = view.mip_count;
		break;
	case TextureViewDimension::Texture2D:
		srv.ViewDimension = D3D12_SRV_DIMENSION_TEXTURE2D;
		srv.Texture2D.MostDetailedMip = view.mip_level;
		srv.Texture2D.MipLevels = view.mip_count;
		srv.Texture2D.PlaneSlice = view.plane;
		break;
	case TextureViewDimension::Texture2DArray:
		srv.ViewDimension = D3D12_SRV_DIMENSION_TEXTURE2DARRAY;
		srv.Texture2DArray.MostDetailedMip = view.mip_level;
		srv.Texture2DArray.MipLevels = view.mip_count;
		srv.Texture2DArray.FirstArraySlice = view.index;
		srv.Texture2DArray.ArraySize = view.depth;
		srv.Texture2DArray.PlaneSlice = view.plane;
		break;
	case TextureViewDimension::Texture2DMS:
		srv.ViewDimension = D3D12_SRV_DIMENSION_TEXTURE2DMS;
		break;
	case TextureViewDimension::Texture3D:
		srv.ViewDimension = D3D12_SRV_DIMENSION_TEXTURE3D;
		srv.Texture3D.MostDetailedMip = view.mip_level;
		srv.Texture3D.MipLevels = view.mip_count;
		break;
	case TextureViewDimension::TextureCube:
		srv.ViewDimension = D3D12_SRV_DIMENSION_TEXTURECUBE;
		srv.Texture3D.MostDetailedMip = view.mip_level;
		srv.Texture3D.MipLevels = view.mip_count;
		break;
	}
	return srv;
}

constexpr D3D12_UNORDERED_ACCESS_VIEW_DESC uav_desc_texture(const TextureView& view) {
	D3D12_UNORDERED_ACCESS_VIEW_DESC uav {};

	uav.Format = dxgi_format(view.format);

	switch(view.dimension) {
	case TextureViewDimension::Texture1D:
		uav.ViewDimension = D3D12_UAV_DIMENSION_TEXTURE1D;
		uav.Texture1D.MipSlice = view.mip_count;
		break;
	case TextureViewDimension::Texture2D:
		uav.ViewDimension = D3D12_UAV_DIMENSION_TEXTURE2D;
		uav.Texture2D.MipSlice = view.mip_level;
		uav.Texture2D.PlaneSlice = view.plane;
		break;
	case TextureViewDimension::Texture2DArray:
		uav.ViewDimension = D3D12_UAV_DIMENSION_TEXTURE2DARRAY;
		uav.Texture2DArray.MipSlice = view.mip_level;
		uav.Texture2DArray.FirstArraySlice = view.index;
		uav.Texture2DArray.ArraySize = view.depth;
		uav.Texture2DArray.PlaneSlice = view.plane;
		break;
	case TextureViewDimension::Texture3D:
		uav.ViewDimension = D3D12_UAV_DIMENSION_TEXTURE3D;
		uav.Texture3D.MipSlice = view.mip_level;
		uav.Texture3D.FirstWSlice = view.index;
		uav.Texture3D.WSize = view.depth;
		break;
	}
	return uav;
}

constexpr D3D12_RESOURCE_STATES d3d12_resource_state(ResourceState state, D3D12_COMMAND_LIST_TYPE type = D3D12_COMMAND_LIST_TYPE_DIRECT) {
	switch(state) {
	case ResourceState::Common:
		return D3D12_RESOURCE_STATE_NON_PIXEL_SHADER_RESOURCE | (type == D3D12_COMMAND_LIST_TYPE_DIRECT ? D3D12_RESOURCE_STATE_PIXEL_SHADER_RESOURCE : D3D12_RESOURCE_STATE_COMMON);
	case ResourceState::RTV:
		return D3D12_RESOURCE_STATE_RENDER_TARGET;
	case ResourceState::UAV:
		return D3D12_RESOURCE_STATE_UNORDERED_ACCESS;
	case ResourceState::DepthWrite:
		return D3D12_RESOURCE_STATE_DEPTH_WRITE;
	case ResourceState::DepthRead:
		return D3D12_RESOURCE_STATE_DEPTH_READ;
	default:
		return D3D12_RESOURCE_STATE_COMMON;
	}
}

struct HeapMemory;

struct Resource {
	HeapMemory* parent;
	ID3D12Resource* resource;
};

struct PipelineState {
	ID3D12PipelineState* pso;
	ID3D12RootSignature* root_signature;
};

struct RenderPass {
	D3D12_RENDER_PASS_RENDER_TARGET_DESC render_targets[max_render_targets];
	D3D12_RENDER_PASS_DEPTH_STENCIL_DESC depth_stencil_target;
	std::uint32_t num_render_targets;
	bool depth_stencil_enable;
};

struct Adapter {
	IDXGIAdapter3* adapter;
	ID3D12Device6* device;
	std::uint32_t node_index;
	DeviceFeatureInfo feature_info;
};

struct SwapChain {
	IDXGISwapChain3* dxgi_swapchain;
	CPUHandle buffers[swapchain_backbuffer_count];
	ID3D12Resource* resources[swapchain_backbuffer_count];
};

struct DescriptorTable {
	CPUHandle cpu_start;
	GPUHandle gpu_start;
	std::uint32_t num_descriptors;
};

class ShaderDescriptorHeap {
public:
	ShaderDescriptorHeap(const Adapter&);
	~ShaderDescriptorHeap();

	DescriptorTable create_descriptor_table(std::uint32_t num_descriptors);
	void destroy_descriptor_table(DescriptorTable&);

	ID3D12DescriptorHeap* get_descriptor_heap() const;
	std::uint32_t get_increment() const;
private:
	ID3D12DescriptorHeap* descriptor_heap;
	GPUHandle start;
	CPUHandle cpu_start;
	std::uint64_t offset;
	std::uint32_t descriptor_count;
	std::uint32_t increment;
	std::vector<DescriptorTable> release_list;
};

class DescriptorPool {
public:
	DescriptorPool(const Adapter&, std::uint32_t num_descriptors, D3D12_DESCRIPTOR_HEAP_TYPE);
	~DescriptorPool();

	CPUHandle add_descriptor();
	void remove_descriptor(CPUHandle);
private:
	const Adapter& adapter;
	std::vector<ID3D12DescriptorHeap*> descriptor_heaps;
	D3D12_DESCRIPTOR_HEAP_DESC desc;

	CPUHandle start;
	std::uint32_t num_descriptors;
	std::uint32_t increment;
	std::vector<CPUHandle> release_list;
};

struct DeviceResources {
	DeviceResources();

	ResourcePool<Resource> buffer_pool;
	ResourcePool<Resource> texture_pool;
	ResourcePool<PipelineState> pipeline_state_pool;
	ResourcePool<DescriptorTable> descriptor_table_pool;
	ResourcePool<RenderPass> render_pass_pool;
};

}