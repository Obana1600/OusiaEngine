#pragma once

#include "Graphics/GraphicsDevice.h"
#include "Graphics/Types/GraphicsCommands.h"
#include "Graphics/Types/GraphicsEnums.h"
#include "Graphics/Types/GraphicsResources.h"

#include <cstdint>
#include <vector>

#include <d3d12.h>
#include <wrl/client.h>


namespace ou::Graphics {

struct CommandQueue {
	D3D12_COMMAND_QUEUE_DESC desc{};
	Microsoft::WRL::ComPtr<ID3D12CommandQueue> queue;

	std::vector<ID3D12CommandList *> submitCmds;

	void Submit();
};


struct CommandList_DX12 {
	Microsoft::WRL::ComPtr<ID3D12CommandAllocator> allocators[GraphicsDevice::BUFFER_COUNT][QUEUE_COUNT];
	Microsoft::WRL::ComPtr<ID3D12GraphicsCommandList> lists[QUEUE_COUNT];

	uint32_t bufferIndex = 0;
	QueueType queue = QUEUE_GRAPHICS;
	uint32_t id = 0;

	GraphicsDevice::GpuLinearAllocator frameAllocators[GraphicsDevice::BUFFER_COUNT];

	std::vector<const SwapChain *> swapChains;
	std::vector<D3D12_RESOURCE_BARRIER> renderPassBarriersEnd;

	const PipelineState *activePso = nullptr;
	RenderPassInfo renderPassInfo;
	bool isPsoDirty = false;

	void Reset(uint32_t newBufferIndex);
	ID3D12CommandAllocator *GetCommandAllocator() const;
	ID3D12GraphicsCommandList *GetGraphicsCommandList() const;
};

} // namespace ou::Graphics