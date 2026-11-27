#pragma once

#include "Graphics/DirectX12/GraphicsAllocators_DX12.h"
#include "Graphics/DirectX12/GraphicsCommands_DX12.h"
#include "Graphics/GraphicsDevice.h"
#include "Graphics/GraphicsTypes.h"

#include <d3d12.h>
#include <dxgi1_6.h>
#include <memory>
#include <mutex>
#include <vector>
#include <wrl/client.h>


namespace ou::Graphics {

class GraphicsDevice_DX12 final: public GraphicsDevice {
public: // メンバ関数

	// コンストラクタ / デストラクタ
	explicit GraphicsDevice_DX12(ValidationMode validationMode = ValidationMode::Disabled);
	~GraphicsDevice_DX12() override;


	// ----- 作成 -----

	bool CreateSwapChain(const SwapChainDesc *desc, HWND hwnd, SwapChain *swapChain) override;
	bool CreateBuffer(const GpuBufferDesc *desc, const void *initialData, GpuBuffer *buffer) override;
	bool CreateTexture(const TextureDesc *desc, const SubresourceData *initialData, Texture *texture) override;
	bool CreateShader(ShaderStage stage, const void *shaderCode, size_t shaderCodeSize, Shader *shader) override;
	bool CreateSampler(const SamplerDesc *desc, Sampler *sampler) override;
	bool CreatePipelineState(const PipelineStateDesc *desc, PipelineState *pso, const RenderPassInfo *renderPassInfo) override;

	void SetName(GpuResource *resource, std::string_view name) const override;

	int GetDescriptorIndex(const GpuResource *resource, ViewType type, int subresource = -1) const override;


	// ----- フレーム -----

	CommandList BeginCommandList(QueueType queue = QUEUE_GRAPHICS) override;

	void SubmitCommandLists() override;
	virtual void WaitForGpu() const override;


	// ----- コマンド -----

	void RenderPassBegin(const SwapChain *swapChain, CommandList cmd) override;
	void RenderPassBegin(const RenderPassImage *images, uint32_t imageCount, CommandList cmd) override;
	void RenderPassEnd(CommandList cmd) override;

	void BindViewports(uint32_t numViewports, const Viewport *viewports, CommandList cmd) override;
	void BindScissorRects(uint32_t numScissors, const Scissor *scissors, CommandList cmd) override;
	void BindPipelineState(const PipelineState *pso, CommandList cmd) override;
	void BindVertexBuffers(const GpuBuffer *const *buffers, uint32_t slot, uint32_t count, const uint32_t *strides, const uint64_t *offsets, CommandList cmd) override;
	void BindIndexBuffer(const GpuBuffer *buffer, IndexBufferFormat format, uint64_t offset, CommandList cmd) override;
	void BindConstantBuffer(const GpuBuffer *buffer, uint32_t slot, CommandList cmd, uint64_t offset = 0) override;
	void BindResource(const GpuResource *resource, uint32_t slot, CommandList cmd, int subresource = -1) override;
	void BindSampler(const Sampler *sampler, uint32_t slot, CommandList cmd) override;
	void BindComputeShader(const Shader *cs, CommandList cmd) override;

	void Draw(uint32_t vertexCount, uint32_t startVertexLocation, CommandList cmd) override;
	void DrawIndexed(uint32_t indexCount, uint32_t startIndexLocation, int32_t baseVertexLocation, CommandList cmd) override;
	void DrawInstanced(uint32_t vertexCount, uint32_t instanceCount, uint32_t startVertexLocation, uint32_t startInstanceLocation, CommandList cmd) override;
	void DrawIndexedInstanced(uint32_t indexCount, uint32_t instanceCount, uint32_t startIndexLocation, int32_t baseVertexLocation, uint32_t startInstanceLocation, CommandList cmd) override;
	void Dispatch(uint32_t threadGroupCountX, uint32_t threadGroupCountY, uint32_t threadGroupCountZ, CommandList cmd) override;

	void CopyBuffer(const GpuBuffer *dst, uint64_t dstOffset, const GpuBuffer *src, uint64_t srcOffset, uint64_t size, CommandList cmd) override;
	void Barrier(const GpuBarrier *barriers, uint32_t numBarriers, CommandList cmd) override;


	// ----- ネイティブ -----

	ID3D12Device *GetNativeDevice() const;
	ID3D12CommandQueue *GetNativeCommandQueue(QueueType queue = QUEUE_GRAPHICS) const;
	ID3D12GraphicsCommandList *GetNativeCommandList(CommandList cmd) const;
	ID3D12DescriptorHeap *GetBindlessHeap() const;

	int AllocateBindlessIndex();
	void FreeBindlessIndex(int index);

	D3D12_CPU_DESCRIPTOR_HANDLE GetBindlessCpuHandle(int index) const;
	D3D12_GPU_DESCRIPTOR_HANDLE GetBindlessGpuHandle(int index) const;


private: // メンバ変数

	ValidationMode validationMode_ = ValidationMode::Disabled;

	Microsoft::WRL::ComPtr<IDXGIFactory6> factory_;
	Microsoft::WRL::ComPtr<IDXGIAdapter4> adapter_;
	Microsoft::WRL::ComPtr<ID3D12Device> device_;

	CommandQueue queues_[QUEUE_COUNT];

	Microsoft::WRL::ComPtr<ID3D12Fence> frameFence_[BUFFER_COUNT][QUEUE_COUNT];

	std::vector<std::unique_ptr<CommandList_DX12>> commandLists_;
	uint32_t cmdCount_ = 0;
	std::mutex cmdLocker_;

	std::shared_ptr<AllocationHandler> allocationHandler_;

	CopyAllocator copyAllocator_;

	// 全 PSO で共有するルートシグネチャ
	Microsoft::WRL::ComPtr<ID3D12RootSignature> rootSignature_;

	// バインドレス用のヒープ
	Microsoft::WRL::ComPtr<ID3D12DescriptorHeap> descriptorHeapRes_;
	uint32_t resDescriptorSize_ = 0;

	// バリア
	std::vector<D3D12_RESOURCE_BARRIER> initTransitions_;
	std::mutex initTransitionLocker_;

	bool tearingSupported_ = false;

	DWORD messageCallbackCookie_ = 0;


private: // メンバ関数

	CommandList_DX12 &GetCommandList(CommandList cmd) const;
	GpuLinearAllocator &GetFrameAllocator(CommandList cmd) override;

	bool ValidatePipelineState(CommandList_DX12 &cmdDX12);


private: // コピー禁止

	GraphicsDevice_DX12(const GraphicsDevice_DX12 &) = delete;
	GraphicsDevice_DX12 &operator=(const GraphicsDevice_DX12 &) = delete;
};

} // namespace ou::Graphics
