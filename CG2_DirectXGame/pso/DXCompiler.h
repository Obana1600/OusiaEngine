#pragma once

#include <wrl.h>
#include <dxcapi.h>
#include <string>


class DXCompiler {
public: // 静的メンバ関数

	static DXCompiler *GetInstance();


public: // メンバ関数

	void Initialize();

	Microsoft::WRL::ComPtr<IDxcBlob> CompileShader(
		const std::wstring &filePath,
		const wchar_t *profile
	);


private: // メンバ変数

	Microsoft::WRL::ComPtr<IDxcUtils> m_dxcUtils{};
	Microsoft::WRL::ComPtr<IDxcCompiler3> m_dxcCompiler{};
	Microsoft::WRL::ComPtr<IDxcIncludeHandler> m_includeHandler{};


private: // メンバ関数

	DXCompiler() = default;
	~DXCompiler() = default;
	DXCompiler(const DXCompiler &) = delete;
	DXCompiler operator=(const DXCompiler &) = delete;

};