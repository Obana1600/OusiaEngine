#pragma once

#include "Graphics/Types/GraphicsCommands.h"
#include "Graphics/Types/GraphicsDescs.h"
#include "Graphics/Types/GraphicsEnums.h"
#include "Graphics/Types/GraphicsResources.h"

#include <Windows.h>
#include <cstdint>
#include <cstring>
#include <string_view>


namespace ou::Graphics {

class GraphicsDevice {
public: // 定数

	static constexpr uint32_t BUFFER_COUNT = 2;
	static constexpr uint32_t DESCRIPTORBINDER_CBV_COUNT = 4;


public: // メンバ関数

	virtual ~GraphicsDevice() = default;


	// ----- 作成 -----

	virtual bool CreateSwapChain(const SwapChainDesc *desc, HWND hwnd, SwapChain *swapChain) = 0;
	virtual bool CreateBuffer(const GpuBufferDesc *desc, const void *initialData, GpuBuffer *buffer) = 0;
	virtual bool CreateTexture(const TextureDesc *desc, const SubresourceData *initialData, Texture *texture) = 0;
	virtual bool CreateShader(ShaderStage stage, const void *shaderCode, size_t shaderCodeSize, Shader *shader) = 0;
	virtual bool CreateSampler(const SamplerDesc *desc, Sampler *sampler) = 0;
	virtual bool CreatePipelineState(const PipelineStateDesc *desc, PipelineState *pso, const RenderPassInfo *renderPassInfo = nullptr) = 0;

	virtual void SetName(GpuResource *resource, std::string_view name) const = 0;

	virtual int GetDescriptorIndex(const GpuResource *resource, ViewType type, int subresource = -1) const = 0;


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
	virtual void BindIndexBuffer(const GpuBuffer *buffer, IndexBufferFormat format, uint64_t offset, CommandList cmd) = 0;
	virtual void BindConstantBuffer(const GpuBuffer *buffer, uint32_t slot, CommandList cmd, uint64_t offset = 0) = 0;
	virtual void BindResource(const GpuResource *resource, uint32_t slot, CommandList cmd, int subresource = -1) = 0;
	virtual void BindSampler(const Sampler *sampler, uint32_t slot, CommandList cmd) = 0;
	virtual void BindComputeShader(const Shader *cs, CommandList cmd) = 0;

	virtual void Draw(uint32_t vertexCount, uint32_t startVertexLocation, CommandList cmd) = 0;
	virtual void DrawIndexed(uint32_t indexCount, uint32_t startIndexLocation, int32_t baseVertexLocation, CommandList cmd) = 0;
	virtual void DrawInstanced(uint32_t vertexCount, uint32_t instanceCount, uint32_t startVertexLocation, uint32_t startInstanceLocation, CommandList cmd) = 0;
	virtual void DrawIndexedInstanced(uint32_t indexCount, uint32_t instanceCount, uint32_t startIndexLocation, int32_t baseVertexLocation, uint32_t startInstanceLocation, CommandList cmd) = 0;
	virtual void Dispatch(uint32_t threadGroupCountX, uint32_t threadGroupCountY, uint32_t threadGroupCountZ, CommandList cmd) = 0;

	virtual void Barrier(const GpuBarrier *barriers, uint32_t numBarriers, CommandList cmd) = 0;


public: // インナークラス

	// 1 フレームだけ使える UPLOAD メモリ
	struct GpuAllocation {
		void *data = nullptr;
		GpuBuffer buffer;
		uint64_t offset = 0;

		bool IsValid() const { return (data != nullptr) && buffer.IsValid(); }
	};

	// コマンドリスト / バッファ番号ごとの線形アロケータ
	struct GpuLinearAllocator {
		GpuBuffer buffer;
		uint64_t offset = 0;

		void Reset() { offset = 0; }
	};


public: // フレームごとの確保

	GpuAllocation AllocateGpu(uint64_t dataSize, CommandList cmd);

	template <typename T>
	void BindDynamicConstantBuffer(const T &data, uint32_t slot, CommandList cmd);


protected: // 描画 API ごとに用意するもの

	virtual GpuLinearAllocator &GetFrameAllocator(CommandList cmd) = 0;

	static constexpr uint64_t kAllocationAlignment = 256;


protected: // メンバ変数

	uint64_t frameCount_ = 0;
};


// グローバルなデバイス
inline GraphicsDevice *&GetDevice() {
	static GraphicsDevice *device = nullptr;
	return device;
}


// value を alignment の倍数に切り上げる
template <typename T>
constexpr T AlignTo(T value, T alignment) {
	return ((value + alignment - 1) / alignment) * alignment;
}


template <typename T>
inline void GraphicsDevice::BindDynamicConstantBuffer(const T &data, uint32_t slot, CommandList cmd) {
	auto allocation = AllocateGpu(sizeof(T), cmd);
	std::memcpy(allocation.data, &data, sizeof(T));
	BindConstantBuffer(&allocation.buffer, slot, cmd, allocation.offset);
}

} // namespace ou::Graphics
