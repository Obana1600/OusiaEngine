#pragma once

#include "Graphics/Types/GraphicsResources.h"

#include <cstdint>
#include <deque>
#include <mutex>
#include <utility>
#include <vector>

#include <d3d12.h>
#include <wrl/client.h>


namespace ou::Graphics {

// 前方宣言
class GraphicsDevice_DX12;


struct DescriptorAllocator {
	ID3D12Device *device = nullptr;
	std::mutex locker;
	D3D12_DESCRIPTOR_HEAP_DESC desc{};
	uint32_t descriptorSize = 0;

	std::vector<Microsoft::WRL::ComPtr<ID3D12DescriptorHeap>> heaps;
	std::vector<D3D12_CPU_DESCRIPTOR_HANDLE> freeList;

	void Initialize(ID3D12Device *d3dDevice, D3D12_DESCRIPTOR_HEAP_TYPE type, uint32_t numDescriptorHeapBlock);
	D3D12_CPU_DESCRIPTOR_HANDLE Allocate();
	void Free(D3D12_CPU_DESCRIPTOR_HANDLE handle);

private:
	void BlockAllocate();
};


struct AllocationHandler {
	std::mutex destroyLocker;
	uint64_t frameCount = 0;

	DescriptorAllocator descriptorRtv;
	DescriptorAllocator descriptorDsv;

	std::deque<std::pair<Microsoft::WRL::ComPtr<ID3D12Resource>, uint64_t>> destroyerResources;
	std::deque<std::pair<Microsoft::WRL::ComPtr<ID3D12PipelineState>, uint64_t>> destroyerPipelines;

	std::vector<int> freeBindlessRes;
	std::deque<std::pair<int, uint64_t>> destroyBindlessRes;

	void Update(uint64_t currentFrameCount, uint32_t bufferCount);
	int AllocateResourceIndex();
};


struct CopyAllocator {
	GraphicsDevice_DX12 *device = nullptr;
	std::mutex locker;

	struct CopyCMD {
		Microsoft::WRL::ComPtr<ID3D12CommandAllocator> allocator;
		Microsoft::WRL::ComPtr<ID3D12GraphicsCommandList> list;
		Microsoft::WRL::ComPtr<ID3D12Fence> fence;
		GpuBuffer uploadBuffer;

		bool IsValid() const {
			return list != nullptr;
		}
	};

	std::vector<CopyCMD> freeList;

	void Initialize(GraphicsDevice_DX12 *owner);
	CopyCMD Allocate(uint64_t stagingSize);
	void Submit(CopyCMD cmd);
};

} // namespace ou::Graphics
