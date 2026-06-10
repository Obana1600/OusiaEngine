#include "VertexBuffer.h"
#include "CreateBufferResource.h"
#include <cassert>
#include <cstring>


VertexBuffer::VertexBuffer(const Microsoft::WRL::ComPtr<ID3D12Device> &device, size_t vertexCount, size_t vertexSize, const void* vertexData) {
	size_t bufferSize = vertexCount * vertexSize;

	m_resource = CreateBufferResource(device, bufferSize);

	m_view.BufferLocation = m_resource->GetGPUVirtualAddress();
	m_view.SizeInBytes = UINT(bufferSize);
	m_view.StrideInBytes = UINT(vertexSize);

	if (vertexData) {
		void *mappedData = nullptr;
		HRESULT hr = m_resource->Map(0, nullptr, &mappedData);
		assert(SUCCEEDED(hr));
		std::memcpy(mappedData, vertexData, bufferSize);
	}

	m_isValid = true;
}