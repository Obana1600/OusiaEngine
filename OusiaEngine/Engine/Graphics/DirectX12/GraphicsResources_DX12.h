#pragma once

#include "Graphics/DirectX12/GraphicsAllocators_DX12.h"
#include "Graphics/Types/GraphicsCommands.h"
#include "Graphics/Types/GraphicsResources.h"

#include <cstdint>
#include <memory>
#include <mutex>
#include <string>
#include <vector>

#include <d3d12.h>
#include <dxgi1_6.h>
#include <wrl/client.h>


namespace ou::Graphics {

struct SwapChain_DX12 {
	std::shared_ptr<AllocationHandler> allocationHandler;

	Microsoft::WRL::ComPtr<IDXGISwapChain4> swapChain;
	std::vector<Microsoft::WRL::ComPtr<ID3D12Resource>> backBuffers;
	std::vector<D3D12_CPU_DESCRIPTOR_HANDLE> backBufferRtvs;

	~SwapChain_DX12();
};


struct Resource_DX12 {
	std::shared_ptr<AllocationHandler> allocationHandler;

	Microsoft::WRL::ComPtr<ID3D12Resource> resource;
	D3D12_GPU_VIRTUAL_ADDRESS gpuAddress = 0;

	int srvIndex = -1;
	int uavIndex = -1;

	virtual ~Resource_DX12();
};


struct Texture_DX12: public Resource_DX12 {
	D3D12_CPU_DESCRIPTOR_HANDLE rtv{};
	D3D12_CPU_DESCRIPTOR_HANDLE dsv{};

	~Texture_DX12();
};


struct Shader_DX12 {
	std::shared_ptr<AllocationHandler> allocationHandler;

	std::vector<uint8_t> shaderCode;

	// コンピュートシェーダだけ
	Microsoft::WRL::ComPtr<ID3D12PipelineState> computePipeline;

	D3D12_SHADER_BYTECODE GetByteCode() const { return {shaderCode.data(), shaderCode.size()}; }

	~Shader_DX12();
};


struct PipelineState_DX12 {
	std::shared_ptr<AllocationHandler> allocationHandler;

	D3D12_GRAPHICS_PIPELINE_STATE_DESC desc{};
	std::vector<D3D12_INPUT_ELEMENT_DESC> inputElements;
	std::vector<std::string> semanticNames;
	std::vector<std::shared_ptr<void>> shaders;

	D3D_PRIMITIVE_TOPOLOGY primitiveTopology = D3D_PRIMITIVE_TOPOLOGY_UNDEFINED;

	// RenderPassInfo ごとの PSO
	struct Variant {
		RenderPassInfo renderPassInfo;
		Microsoft::WRL::ComPtr<ID3D12PipelineState> pipelineState;
	};
	std::vector<Variant> variants;
	std::mutex variantsLocker;

	~PipelineState_DX12();
};


SwapChain_DX12 *ToInternal(const SwapChain *param);

Resource_DX12 *ToInternal(const GpuResource *param);

Texture_DX12 *ToInternal(const Texture *param);

Shader_DX12 *ToInternal(const Shader *param);

PipelineState_DX12 *ToInternal(const PipelineState *param);

ID3D12PipelineState *FindOrCreatePipeline(ID3D12Device *device,
										  PipelineState_DX12 &is,
										  const RenderPassInfo &renderPassInfo);

} // namespace ou::Graphics
