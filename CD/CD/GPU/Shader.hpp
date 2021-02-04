#pragma once

#include <CD/GPU/Common.hpp>
#include <memory>
#include <wrl/client.h>
#include <dxc/Support/dxcapi.use.h>

namespace CD::GPU {

enum class ShaderStage {
	Compute,
	Vertex,
	Pixel,
};

struct CompileShaderDesc {
	const wchar_t* path;
	const wchar_t* entry_point;
	ShaderStage profile;
	const wchar_t** defines;
	std::size_t num_defines;
};

struct ShaderDeleter {
	void operator()(IDxcBlob* shader) {
		if(shader) {
			shader->Release();
		}
	}
};

using ShaderPtr = std::unique_ptr<IDxcBlob, ShaderDeleter>;

class ShaderCompiler {
public:
	ShaderCompiler();
	~ShaderCompiler();

	ShaderPtr compile_shader(const CompileShaderDesc&);
private:
	dxc::DxcDllSupport dll_helper;
	IDxcUtils* utils;
	IDxcCompiler3* compiler;
	IDxcIncludeHandler* include_handler;
};

}