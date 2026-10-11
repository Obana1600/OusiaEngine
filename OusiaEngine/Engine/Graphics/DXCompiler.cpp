#include "DXCompiler.h"
#include "Core/Fatal.h"
#include "Core/Paths.h"

#include "Graphics/GraphicsDevice.h"

#include <cassert>

#pragma comment(lib, "dxcompiler.lib")

using namespace ou;


DXCompiler *DXCompiler::GetInstance() {
	static DXCompiler instance{};
	return &instance;
}


void DXCompiler::Initialize() {
	// dxCompilerを初期化
	auto hr = DxcCreateInstance(CLSID_DxcUtils, IID_PPV_ARGS(&dxcUtils_));
	CheckHResult(hr, "DxcUtils の作成");
	hr = DxcCreateInstance(CLSID_DxcCompiler, IID_PPV_ARGS(&dxcCompiler_));
	CheckHResult(hr, "DxcCompiler の作成");

	// 現時点でincludeはしないが、includeに対応するための設定を行っておく
	hr = dxcUtils_->CreateDefaultIncludeHandler(&includeHandler_);
	CheckHResult(hr, "DxcIncludeHandler の作成");
}


Shader ou::DXCompiler::CompileEngineShader(std::wstring_view fileName, ShaderStage stage) {
	const auto engineAssets = FindEngineAssetsFolder();
	if (!engineAssets) {
		FatalError("EngineAssets フォルダが見つかりません");
	}

	const auto *profile = (stage == ShaderStage::VS) ? L"vs_6_6" : L"ps_6_6";
	auto blob = GetInstance()->CompileShaderInternal(std::wstring(*engineAssets / L"Shader" / fileName), profile);

	Shader shader{};
	if (!GetDevice()->CreateShader(stage, blob->GetBufferPointer(), blob->GetBufferSize(), &shader)) {
		FatalError("エンジンのシェーダの作成に失敗しました。");
	}

	return shader;
}


Microsoft::WRL::ComPtr<IDxcBlob> ou::DXCompiler::CompileShaderInternal(std::wstring_view filePath, const wchar_t *profile) {
	const auto &key = std::wstring(filePath);

	// hlslファイルを読む
	Microsoft::WRL::ComPtr<IDxcBlobEncoding> shaderSource{};
	auto hr = dxcUtils_->LoadFile(key.c_str(), nullptr, &shaderSource);
	CheckHResult(hr, "HLSL の読み込み");

	// 読み込んだファイルの内容を設定する
	DxcBuffer shaderSourceBuffer{};
	shaderSourceBuffer.Ptr = shaderSource->GetBufferPointer();
	shaderSourceBuffer.Size = shaderSource->GetBufferSize();
	shaderSourceBuffer.Encoding = DXC_CP_UTF8; // UTF8の文字コードであることを通知

	LPCWSTR arguments[] = {
		key.c_str(), // コンパイル対象のhlslファイル名
		L"-E",
		L"main", // エントリーポイントの指定。基本的にmain以外にはしない
		L"-T",
		profile, // ShaderProfileの設定
		L"-Zi",
		L"-Qembed_debug", // デバッグ用の情報を埋め込む
		L"-Od",			  // 最適化を外しておく
		L"-Zpr",		  // メモリレイアウトは行優先
	};
	// 実際にShaderをコンパイルする
	Microsoft::WRL::ComPtr<IDxcResult> shaderResult{};
	hr = dxcCompiler_->Compile(
		&shaderSourceBuffer,		// 読み込んだファイル
		arguments,					// コンパイルオプション
		_countof(arguments),		// コンパイルオプションの数
		includeHandler_.Get(),		// includeが含まれた諸々
		IID_PPV_ARGS(&shaderResult) // コンパイル結果
	);
	CheckHResult(hr, "シェーダのコンパイル");

	// 警告・エラーが出てたらログに出して止める
	Microsoft::WRL::ComPtr<IDxcBlobUtf8> shaderError{};
	shaderResult->GetOutput(DXC_OUT_ERRORS, IID_PPV_ARGS(&shaderError), nullptr);
	if (shaderError && shaderError->GetStringLength() != 0) {
		OutputDebugStringA(shaderError->GetStringPointer());
		assert(false);
	}

	// コンパイル結果から実行用のバイナリ部分を取得
	Microsoft::WRL::ComPtr<IDxcBlob> shaderBlob{};
	hr = shaderResult->GetOutput(DXC_OUT_OBJECT, IID_PPV_ARGS(&shaderBlob), nullptr);
	CheckHResult(hr, "シェーダコードの取得");

	// 実行用のバイナリを返却
	return shaderBlob;
}
