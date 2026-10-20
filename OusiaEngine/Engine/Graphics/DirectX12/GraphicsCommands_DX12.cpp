#include "GraphicsCommands_DX12.h"


void ou::Graphics::CommandQueue::Submit() {
	if (submitCmds.empty()) { return; }

	queue->ExecuteCommandLists(static_cast<UINT>(submitCmds.size()), submitCmds.data());
	submitCmds.clear();
}


void ou::Graphics::CommandList_DX12::Reset(uint32_t newBufferIndex) {
	bufferIndex = newBufferIndex;
	swapChains.clear();
	renderPassBarriersEnd.clear();
	frameAllocators[newBufferIndex].Reset();
	activePso = nullptr;
	renderPassInfo = {};
	isPsoDirty = false;
}


ID3D12CommandAllocator *ou::Graphics::CommandList_DX12::GetCommandAllocator() const {
	return allocators[bufferIndex][queue].Get();
}


ID3D12GraphicsCommandList *ou::Graphics::CommandList_DX12::GetGraphicsCommandList() const {
	return lists[queue].Get();
}