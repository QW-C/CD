#pragma once

#include <CD/GPU/Utils.hpp>
#include <CD/GPU/D3D12/Common.hpp>
#include <DirectXTex/DirectXTex.h>

namespace CD::GPU {

BufferFormat typed_depth_format(BufferFormat format) {
	switch(format) {
	case BufferFormat::R24G8_Typeless:
		return BufferFormat::D24_UNORM_S8_UINT;
	case BufferFormat::R32_FLOAT_X8X24_Typeless:
		return BufferFormat::D32_FLOAT;
	default:
		return BufferFormat::Unused;
	}
}

bool is_depth_stencil_format(BufferFormat format) {
	return format == BufferFormat::D16_UNORM ||
		format == BufferFormat::D24_UNORM_S8_UINT ||
		format == BufferFormat::D32_FLOAT ||
		format == BufferFormat::D32_FLOAT_S8X24_UINT;
}

std::uint32_t row_size(std::uint32_t size, BufferFormat format) {
	return static_cast<std::uint32_t>((size * DirectX::BitsPerPixel(D3D12::dxgi_format(format)) + 7) / 8);
}

}