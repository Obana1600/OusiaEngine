#pragma once

#include "Graphics/GraphicsDevice.h"

#include <d3d12.h>
#include <dxgi1_6.h>
#include <wrl/client.h>


namespace ou {

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
	bool CreatePipelineState(const PipelineStateDesc *desc, PipelineState *pso) override;

	void SetName(GpuResource *resource, std::string_view name) const override;


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
	void BindIndexBuffer(const GpuBuffer *buffer, IndexBufferFormat format, uint64_t offsets, CommandList cmd) override;
	void BindConstantBuffer(const GpuBuffer *buffer, uint32_t slot, CommandList cmd, uint64_t offset = 0) override;
	void BindResource(const GpuResource *resource, uint32_t slot, CommandList cmd, int subresource = -1) override;
	void BindSampler(const Sampler *sampler, uint32_t slot, CommandList cmd) override;

	void Draw(uint32_t vertexCount, uint32_t startVertexLocation, CommandList cmd) override;
	void DrawIndexed(uint32_t indexCount, uint32_t startIndexLocation, int32_t baseVertexLocation, CommandList cmd) override;
	void DrawInstanced(uint32_t vertexCount, uint32_t instanceCount, uint32_t startVertexLocation, uint32_t startInstanceLocation, CommandList cmd) override;
	void DrawIndexedInstanced(uint32_t indexCount, uint32_t instanceCount, uint32_t startIndexLocation, int32_t baseVertexLocation, uint32_t startInstanceLocation, CommandList cmd) override;

	void Barrier(const GpuBarrier *barriers, uint32_t numBarriers, CommandList cmd) override;


private: // メンバ変数
private: // コピー禁止

	GraphicsDevice_DX12(const GraphicsDevice_DX12 &) = delete;
	GraphicsDevice_DX12 &operator=(const GraphicsDevice_DX12 &) = delete;
};

} // namespace ou
