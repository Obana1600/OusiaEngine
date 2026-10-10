#pragma once

#include <cstdint>
#include <d3d12.h>
#include <queue>
#include <utility>
#include <vector>
#include <wrl/client.h>


namespace ou {

class CommandAllocatorPool {
public: // メンバ関数

	CommandAllocatorPool(ID3D12Device *device, D3D12_COMMAND_LIST_TYPE type);

	// プールから再利用し、なければ新しく作る
	ID3D12CommandAllocator *Request(uint64_t completedFenceValue);

	// fenceValue を通過したら再利用できるよう返却する
	void Discard(uint64_t fenceValue, ID3D12CommandAllocator *allocator);

	D3D12_COMMAND_LIST_TYPE Type() const;


private: // メンバ変数

	std::vector<Microsoft::WRL::ComPtr<ID3D12CommandAllocator>> allocatorPool_;
	std::queue<std::pair<uint64_t, ID3D12CommandAllocator *>> readyAllocators_;
	// TODO: mutex 対応

	ID3D12Device *device_ = nullptr;
	const D3D12_COMMAND_LIST_TYPE type_{};


private: // コピー禁止

	CommandAllocatorPool(const CommandAllocatorPool &) = delete;
	CommandAllocatorPool &operator=(const CommandAllocatorPool &) = delete;
};

} // namespace ou