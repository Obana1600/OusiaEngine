#pragma once

#include <wrl.h>
#include <d3d12.h>


class RootSignature {
public: // メンバ関数

	RootSignature(const Microsoft::WRL::ComPtr<ID3D12Device> &device);


	ID3D12RootSignature *Get() {
		return m_rootSignature.Get();
	}

	bool IsValid() const {
		return m_isValid;
	}


private: // メンバ変数

	Microsoft::WRL::ComPtr<ID3D12RootSignature> m_rootSignature{};

	bool m_isValid = false;

};