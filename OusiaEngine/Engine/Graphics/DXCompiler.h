#pragma once

#include "Graphics/GraphicsTypes.h"

#include <wrl/client.h>
#include <dxcapi.h> // wrl/client.h より後ろに置く
#include <string_view>


namespace ou {

class DXCompiler {
public: // 静的メンバ関数

	static DXCompiler *GetInstance();


public: // メンバ関数

	void Initialize();

	Shader CompileEngineShader(std::wstring_view fileName, ShaderStage stage);


private: // メンバ変数

	Microsoft::WRL::ComPtr<IDxcUtils> dxcUtils_;
	Microsoft::WRL::ComPtr<IDxcCompiler3> dxcCompiler_;
	Microsoft::WRL::ComPtr<IDxcIncludeHandler> includeHandler_;


private: // メンバ関数

	Microsoft::WRL::ComPtr<IDxcBlob> CompileShaderInternal(
		std::wstring_view filePath, const wchar_t *profile);


private: // コンストラクタ

	DXCompiler() = default;
	~DXCompiler() = default;
	DXCompiler(const DXCompiler &) = delete;
	DXCompiler operator=(const DXCompiler &) = delete;
};

} // namespace ou
