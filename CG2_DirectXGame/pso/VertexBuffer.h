#pragma once

#include <d3d12.h>
#include <wrl.h>


class VertexBuffer {
public: // メンバ関数

	VertexBuffer(const Microsoft::WRL::ComPtr<ID3D12Device> &device, size_t vertexCount, size_t vertexSize, const void *vertexData);


	D3D12_VERTEX_BUFFER_VIEW View() const {
		return m_view;
	}

	bool IsValid() const {
		return m_isValid;
	}


private: // メンバ変数

	Microsoft::WRL::ComPtr<ID3D12Resource> m_resource{};

	D3D12_VERTEX_BUFFER_VIEW m_view{};

	bool m_isValid = false;


private: // メンバ関数

	VertexBuffer(const VertexBuffer &) = delete;
	VertexBuffer operator=(const VertexBuffer &) = delete;

};