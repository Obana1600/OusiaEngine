#pragma once

#include "Graphics/GraphicsTypes.h"

#include <Windows.h>
#include <cstdint>
#include <string_view>

// TODO: ShaderModel6.6~ の bindless に対応させたい

namespace ou {

class GraphicsDevice {
public: // 定数

	static constexpr uint32_t BUFFER_COUNT = 2;


public: // メンバ関数

	virtual ~GraphicsDevice() = default;


	// ----- 作成 -----

	virtual bool CreateSwapChain(const SwapChainDesc *desc, HWND hwnd, SwapChain *swapChain) = 0;
	virtual bool CreateBuffer(const GpuBufferDesc *desc, const void *initialData, GpuBuffer *buffer) = 0;
	virtual bool CreateTexture(const TextureDesc *desc, const SubresourceData *initialData, Texture *texture) = 0;
	virtual bool CreateShader(ShaderStage stage, const void *shaderCode, size_t shaderCodeSize, Shader *shader) = 0;
	virtual bool CreateSampler(const SamplerDesc *desc, Sampler *sampler) = 0;
	virtual bool CreatePipelineState(const PipelineStateDesc *desc, PipelineState *pso) = 0;

	virtual void SetName(GpuResource *resource, std::string_view name) const = 0;


	// ----- フレーム -----

	virtual CommandList BeginCommandList(QueueType queue = QUEUE_GRAPHICS) = 0;

	virtual void SubmitCommandLists() = 0;

	virtual void WaitForGpu() const = 0;

	uint64_t GetFrameCount() const { return frameCount_; }

	uint32_t GetBufferIndex() const { return static_cast<uint32_t>(frameCount_ % BUFFER_COUNT); }

	static constexpr uint32_t GetBufferCount() { return BUFFER_COUNT; }


	// ----- コマンド -----

	virtual void RenderPassBegin(const SwapChain *swapChain, CommandList cmd) = 0;
	virtual void RenderPassBegin(const RenderPassImage *images, uint32_t imageCount, CommandList cmd) = 0;
	virtual void RenderPassEnd(CommandList cmd) = 0;

	virtual void BindViewports(uint32_t numViewports, const Viewport *viewports, CommandList cmd) = 0;
	virtual void BindScissorRects(uint32_t numScissors, const Scissor *scissors, CommandList cmd) = 0;
	virtual void BindPipelineState(const PipelineState *pso, CommandList cmd) = 0;
	virtual void BindVertexBuffers(const GpuBuffer *const *buffers, uint32_t slot, uint32_t count, const uint32_t *strides, const uint64_t *offsets, CommandList cmd) = 0;
	virtual void BindIndexBuffer(const GpuBuffer *buffer, IndexBufferFormat format, uint64_t offsets, CommandList cmd) = 0;
	virtual void BindConstantBuffer(const GpuBuffer *buffer, uint32_t slot, CommandList cmd, uint64_t offset = 0) = 0;
	virtual void BindResource(const GpuResource *resource, uint32_t slot, CommandList cmd, int subresource = -1) = 0;
	virtual void BindSampler(const Sampler *sampler, uint32_t slot, CommandList cmd) = 0;

	virtual void Draw(uint32_t vertexCount, uint32_t startVertexLocation, CommandList cmd) = 0;
	virtual void DrawIndexed(uint32_t indexCount, uint32_t startIndexLocation, int32_t baseVertexLocation, CommandList cmd) = 0;
	virtual void DrawInstanced(uint32_t vertexCount, uint32_t instanceCount, uint32_t startVertexLocation, uint32_t startInstanceLocation, CommandList cmd) = 0;
	virtual void DrawIndexedInstanced(uint32_t indexCount, uint32_t instanceCount, uint32_t startIndexLocation, int32_t baseVertexLocation, uint32_t startInstanceLocation, CommandList cmd) = 0;

	virtual void Barrier(const GpuBarrier *barriers, uint32_t numBarriers, CommandList cmd) = 0;


protected: // メンバ変数

	uint64_t frameCount_ = 0;
};


// グローバルなデバイス
inline GraphicsDevice *&GetDevice() {
	static GraphicsDevice *device = nullptr;
	return device;
}

} // namespace ou
