#include "IndexBuffer.h"
#include "CreateBufferResource.h"
#include <cstring>


IndexBuffer::IndexBuffer(const Microsoft::WRL::ComPtr<ID3D12Device> &device, size_t indexCount, const uint32_t *indexData) {
	size_t bufferSize = sizeof(uint32_t) * indexCount;

	m_resource = CreateBufferResource(device, bufferSize);

	m_view.BufferLocation = m_resource->GetGPUVirtualAddress();
	m_view.SizeInBytes = UINT(bufferSize);
	m_view.Format = DXGI_FORMAT_R32_UINT;

	if (indexData) {
		void *mappedData = nullptr;
		HRESULT hr = m_resource->Map(0, nullptr, &mappedData);
		assert(SUCCEEDED(hr));
		std::memcpy(mappedData, indexData, bufferSize);
	}

	m_isValid = true;
}