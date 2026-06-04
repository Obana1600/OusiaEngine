#pragma once

#include <wrl.h>
#include <d3d12.h>


class ConstBuffer {
public: // メンバ関数

	ConstBuffer(const Microsoft::WRL::ComPtr<ID3D12Device> &device, size_t size, const void* data);

	D3D12_GPU_VIRTUAL_ADDRESS GetAddress() const {
		return m_resource->GetGPUVirtualAddress();
	}

	D3D12_CONSTANT_BUFFER_VIEW_DESC ViewDesc() const {
		return m_desc;
	}

	bool IsValid() const {
		return m_isValid;
	}


private: // メンバ変数

	Microsoft::WRL::ComPtr<ID3D12Resource> m_resource{};

	D3D12_CONSTANT_BUFFER_VIEW_DESC m_desc{};

	bool m_isValid = false;


private: // メンバ関数

	ConstBuffer(const ConstBuffer &) = delete;
	ConstBuffer operator=(const ConstBuffer &) = delete;

};