#include "ShaderCompiler.h"
#include "Core/Paths.h"

// clang-format off
#include <wrl/client.h>
#include <dxcapi.h> // wrl/client.h より後ろに置く
// clang-format on

#pragma comment(lib, "dxcompiler.lib")

using namespace ou;
using namespace Microsoft::WRL;


namespace {

struct InternalState_DXC {
	ComPtr<IDxcUtils> utils;
	ComPtr<IDxcCompiler3> compiler;
	ComPtr<IDxcIncludeHandler> includeHandler;

	InternalState_DXC();
};

InternalState_DXC::InternalState_DXC() {
	DxcCreateInstance(CLSID_DxcUtils, IID_PPV_ARGS(&utils));
	DxcCreateInstance(CLSID_DxcCompiler, IID_PPV_ARGS(&compiler));
	if (utils) {
		utils->CreateDefaultIncludeHandler(&includeHandler);
	}
}

InternalState_DXC &DxcCompiler() {
	static InternalState_DXC dxc;
	return dxc;
}

struct CompilerOutput_DXC {
	ComPtr<IDxcBlob> blob;
};

std::wstring MakeProfile(Graphics::ShaderStage stage, Graphics::ShaderModel model) {
	const wchar_t *stageName = nullptr;
	switch (stage) {
		case Graphics::ShaderStage::MS:
			stageName = L"ms";
			break;
		case Graphics::ShaderStage::AS:
			stageName = L"as";
			break;
		case Graphics::ShaderStage::HS:
			stageName = L"hs";
			break;
		case Graphics::ShaderStage::DS:
			stageName = L"ds";
			break;
		case Graphics::ShaderStage::GS:
			stageName = L"gs";
			break;
		case Graphics::ShaderStage::VS:
			stageName = L"vs";
			break;
		case Graphics::ShaderStage::PS:
			stageName = L"ps";
			break;
		case Graphics::ShaderStage::CS:
			stageName = L"cs";
			break;
		case Graphics::ShaderStage::LIB:
			stageName = L"lib";
			break;
		default:
			return {};
	}

	const wchar_t *modelName = nullptr;
	switch (model) {
		case Graphics::ShaderModel::SM_6_6:
			modelName = L"6_6";
			break;
		case Graphics::ShaderModel::SM_6_7:
			modelName = L"6_7";
			break;
		case Graphics::ShaderModel::SM_6_8:
			modelName = L"6_8";
			break;
		case Graphics::ShaderModel::SM_6_9:
			modelName = L"6_9";
			break;
		default:
			return {};
	}

	return std::wstring(stageName) + L"_" + modelName;
}

} // namespace



ShaderCompiler::CompilerOutput ou::ShaderCompiler::Compile(const CompilerInput &input) {
	CompilerOutput output;

	auto &dxc = DxcCompiler();
	if (!dxc.compiler || !dxc.includeHandler) {
		output.errorMessage = "DXC の初期化に失敗しました";
		return output;
	}

	const auto profile = MakeProfile(input.stage, input.minShaderModel);
	if (profile.empty()) {
		output.errorMessage = "対応していないシェーダステージかシェーダモデルです";
		return output;
	}

	// hlsl ファイルを読む
	const auto path = Utf8ToWide(input.shaderPath);
	ComPtr<IDxcBlobEncoding> source;
	auto hr = dxc.utils->LoadFile(path.c_str(), nullptr, &source);
	if (FAILED(hr)) {
		output.errorMessage = "HLSL を読めませんでした: " + input.shaderPath;
		return output;
	}

	DxcBuffer sourceBuffer{};
	sourceBuffer.Ptr = source->GetBufferPointer();
	sourceBuffer.Size = source->GetBufferSize();
	sourceBuffer.Encoding = DXC_CP_UTF8;

	std::vector<std::wstring> arguments;
	arguments.push_back(path);
	arguments.push_back(L"-E");
	arguments.push_back(Utf8ToWide(input.entryPoint));
	arguments.push_back(L"-T");
	arguments.push_back(profile);
	arguments.push_back(L"-Zpr"); // 行優先
	for (const auto &directory : input.includeDirectories) {
		arguments.push_back(L"-I");
		arguments.push_back(Utf8ToWide(directory));
	}
	for (const auto &define : input.defines) {
		arguments.push_back(L"-D");
		arguments.push_back(Utf8ToWide(define));
	}
	if (HasFlag(input.flags, Flags::DISABLE_OPTIMIZATION)) {
		arguments.push_back(L"-Od");
	}
	if (HasFlag(input.flags, Flags::EMBED_DEBUG_INFO)) {
		arguments.push_back(L"-Zi");
		arguments.push_back(L"-Qembed_debug");
	}

	// wstring -> wchar_t
	std::vector<LPCWSTR> argumentPointers;
	argumentPointers.reserve(arguments.size());
	for (const auto &argument : arguments) {
		argumentPointers.push_back(argument.c_str());
	}

	ComPtr<IDxcResult> result;
	hr = dxc.compiler->Compile(
		&sourceBuffer,
		argumentPointers.data(),
		static_cast<UINT32>(argumentPointers.size()),
		dxc.includeHandler.Get(),
		IID_PPV_ARGS(&result));
	if (FAILED(hr)) {
		output.errorMessage = "DXC の呼び出しに失敗しました";
		return output;
	}

	// エラー / 警告
	ComPtr<IDxcBlobUtf8> errors;
	result->GetOutput(DXC_OUT_ERRORS, IID_PPV_ARGS(&errors), nullptr);
	if (errors && errors->GetStringLength() != 0) {
		output.errorMessage = errors->GetStringPointer();
	}

	// コンパイル自体の成否
	HRESULT status = S_OK;
	result->GetStatus(&status);
	if (FAILED(status)) {
		return output;
	}

	ComPtr<IDxcBlob> blob;
	result->GetOutput(DXC_OUT_OBJECT, IID_PPV_ARGS(&blob), nullptr);
	if (!blob) {
		return output;
	}

	auto is = std::make_shared<CompilerOutput_DXC>();
	is->blob = blob;
	output.shaderData = static_cast<const uint8_t *>(blob->GetBufferPointer());
	output.shaderSize = blob->GetBufferSize();
	output.internalState = is;
	return output;
}
