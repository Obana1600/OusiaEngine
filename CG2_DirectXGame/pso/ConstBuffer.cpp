#include "ConstBuffer.h"
#include "CreateBufferResource.h"
#include <cassert>
#include <cstring>


ConstBuffer::ConstBuffer(const Microsoft::WRL::ComPtr<ID3D12Device> &device, size_t size, const void *data) {
	size_t align = D3D12_CONSTANT_BUFFER_DATA_PLACEMENT_ALIGNMENT;
	size_t sizeAligned = (size + (align - 1) & ~(align - 1));

	m_resource = CreateBufferResource(device, sizeAligned);

	if (data) {
		void *mappedData = nullptr;
		HRESULT hr = m_resource->Map(0, nullptr, &mappedData);
		assert(SUCCEEDED(hr));
		std::memcpy(mappedData, data, size);
	}

	m_desc.BufferLocation = m_resource->GetGPUVirtualAddress();
	m_desc.SizeInBytes = UINT(sizeAligned);
	m_isValid = true;
}