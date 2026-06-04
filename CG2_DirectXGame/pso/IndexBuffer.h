#pragma once

#include <wrl.h>
#include <d3d12.h>
#include <cstdint>


class IndexBuffer {
public: // メンバ関数

	IndexBuffer(const Microsoft::WRL::ComPtr<ID3D12Device> &device, size_t indexCount, const uint32_t *indexData);


	D3D12_INDEX_BUFFER_VIEW View() const {
		return m_view;
	}

	bool IsValid() const {
		return m_isValid;
	}


private: // メンバ変数

	Microsoft::WRL::ComPtr<ID3D12Resource> m_resource{};

	D3D12_INDEX_BUFFER_VIEW m_view{};

	bool m_isValid = false;


private: // メンバ関数

	IndexBuffer(const IndexBuffer &) = delete;
	IndexBuffer operator=(const IndexBuffer &) = delete;

};