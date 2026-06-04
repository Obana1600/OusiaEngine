#pragma once

#include <d3d12.h>
#include <wrl.h>
#include <string>
#include <dxcapi.h>


class PipelineState {
public: // メンバ関数

	PipelineState();


	void SetInputLayout(D3D12_INPUT_LAYOUT_DESC layout) {
		m_desc.InputLayout = layout;
	}

	void SetRootSignature(ID3D12RootSignature *rootSignature) {
		m_desc.pRootSignature = rootSignature;
	}

	void SetVS(const std::wstring &filePath, const wchar_t *profile);

	void SetPS(const std::wstring &filePath, const wchar_t *profile);

	void Create(const Microsoft::WRL::ComPtr<ID3D12Device> &device);


	bool IsValid() const {
		return m_isValid;
	}


private: // メンバ変数

	D3D12_GRAPHICS_PIPELINE_STATE_DESC m_desc{};

	Microsoft::WRL::ComPtr<ID3D12PipelineState> m_pipelineState;

	Microsoft::WRL::ComPtr<IDxcBlob> m_VSBlob;

	Microsoft::WRL::ComPtr<IDxcBlob> m_PSBlob;

	bool m_isValid = false;

};