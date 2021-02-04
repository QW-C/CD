#include <CD/GPU/Shader.hpp>
#include <CD/Common/Debug.hpp>
#include <vector>

namespace CD::GPU {

using Microsoft::WRL::ComPtr;

#define ASSERT_SUCCEEDED(HR) CD_ASSERT(HR >= 0)

constexpr const wchar_t* shader_profiles[] {
	L"cs_6_0",
	L"vs_6_0",
	L"ps_6_0",
};

ShaderCompiler::ShaderCompiler() {
	ASSERT_SUCCEEDED(dll_helper.Initialize());
	ASSERT_SUCCEEDED(dll_helper.CreateInstance<IDxcUtils>(CLSID_DxcUtils, &utils));
	ASSERT_SUCCEEDED(dll_helper.CreateInstance<IDxcCompiler3>(CLSID_DxcCompiler, &compiler));
	ASSERT_SUCCEEDED(utils->CreateDefaultIncludeHandler(&include_handler));
}

ShaderCompiler::~ShaderCompiler() {
	utils->Release();
	compiler->Release();
	include_handler->Release();
}

ShaderPtr ShaderCompiler::compile_shader(const CompileShaderDesc& desc) {
	std::vector<const wchar_t*> args;
	args.push_back(L"-E");
	args.push_back(desc.entry_point);
	args.push_back(L"-T");
	args.push_back(shader_profiles[static_cast<std::size_t>(desc.profile)]);
	args.push_back(DXC_ARG_DEBUG);
	args.push_back(DXC_ARG_PACK_MATRIX_ROW_MAJOR);

	for(std::size_t i = 0; i < desc.num_defines; ++i) {
		args.push_back(L"-D");
		args.push_back(desc.defines[i]);
	}

	std::uint32_t code_page = CP_UTF8;
	ComPtr<IDxcBlobEncoding> source;
	ASSERT_SUCCEEDED(utils->LoadFile(desc.path, &code_page, &source));
	DxcBuffer source_buffer {source->GetBufferPointer(), source->GetBufferSize()};

	ComPtr<IDxcResult> result;
	ASSERT_SUCCEEDED(compiler->Compile(
		&source_buffer,
		args.data(),
		static_cast<UINT>(args.size()),
		include_handler,
		IID_PPV_ARGS(&result)
	));

	ComPtr<IDxcBlobUtf8> errors;
	ASSERT_SUCCEEDED(result->GetOutput(DXC_OUT_ERRORS, IID_PPV_ARGS(&errors), nullptr));
	if(errors && errors->GetBufferSize()) {
		CD_FAIL(errors->GetStringPointer());
	}

	IDxcBlob* shader = nullptr;
	ASSERT_SUCCEEDED(result->GetOutput(DXC_OUT_OBJECT, IID_PPV_ARGS(&shader), nullptr));

	return ShaderPtr(shader);
}

}