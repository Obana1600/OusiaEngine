#pragma once

#include <wrl.h>
#include <d3d12.h>


class ConstBuffer {
public: // メンバ関数

	ConstBuffer(const Microsoft::WRL::ComPtr<ID3D12Device> &device, size_t size, const void* data = nullptr);
	~ConstBuffer() = default;

	D3D12_GPU_VIRTUAL_ADDRESS GetAddress() const {
		return m_resource->GetGPUVirtualAddress();
	}

	D3D12_CONSTANT_BUFFER_VIEW_DESC ViewDesc() const {
		return m_desc;
	}

	bool IsValid() const {
		return m_isValid;
	}

	/// マップ済みポインタを型付きで取得（元のreinterpret_castと同じ使い方）
	template<typename T>
	T *GetData() const {
		return reinterpret_cast<T *>(m_mappedData);
	}


private: // メンバ変数

	Microsoft::WRL::ComPtr<ID3D12Resource> m_resource{};

	D3D12_CONSTANT_BUFFER_VIEW_DESC m_desc{};

	void *m_mappedData = nullptr;

	bool m_isValid = false;


private: // メンバ関数

	ConstBuffer(const ConstBuffer &) = delete;
	ConstBuffer operator=(const ConstBuffer &) = delete;

};