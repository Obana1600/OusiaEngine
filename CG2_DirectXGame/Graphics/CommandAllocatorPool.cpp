#include "CommandAllocatorPool.h"
#include "Core/Fatal.h"

#include <utility>

using namespace ou;
using namespace Microsoft::WRL;


ou::CommandAllocatorPool::CommandAllocatorPool(ID3D12Device *device, D3D12_COMMAND_LIST_TYPE type)
	: device_(device),
	  type_(type) {}


ID3D12CommandAllocator *ou::CommandAllocatorPool::Request(uint64_t completedFenceValue) {
	// 返却待ちのアロケータの先頭を調べる
	if (!readyAllocators_.empty()) {
		auto [fenceValue, allocator] = readyAllocators_.front();

		// GPU が使い終わっているなら、それを返す
		if (fenceValue <= completedFenceValue) {
			readyAllocators_.pop();
			auto hr = allocator->Reset();
			CheckHResult(hr, "コマンドアロケータのリセット");
			return allocator;
		}
	}

	// 再利用できないなら新規作成
	ComPtr<ID3D12CommandAllocator> allocator;
	auto hr = device_->CreateCommandAllocator(type_, IID_PPV_ARGS(&allocator));
	CheckHResult(hr, "コマンドアロケータの作成");
	allocatorPool_.push_back(allocator);

	return allocator.Get();
}


void ou::CommandAllocatorPool::Discard(uint64_t fenceValue, ID3D12CommandAllocator *allocator) {
	readyAllocators_.push(std::pair(fenceValue, allocator));
}


D3D12_COMMAND_LIST_TYPE ou::CommandAllocatorPool::Type() const {
	return type_;
}
