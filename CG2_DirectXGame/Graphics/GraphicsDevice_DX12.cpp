#include "GraphicsDevice_DX12.h"

using namespace ou;

namespace {

}



ou::GraphicsDevice_DX12::GraphicsDevice_DX12(ValidationMode validationMode) {
	(void)validationMode;
}


ou::GraphicsDevice_DX12::~GraphicsDevice_DX12() {
}


bool ou::GraphicsDevice_DX12::CreateSwapChain(const SwapChainDesc *desc, HWND hwnd, SwapChain *swapChain) {
	(void)desc;
	(void)hwnd;
	(void)swapChain;
	return false;
}


bool ou::GraphicsDevice_DX12::CreateBuffer(const GpuBufferDesc *desc, const void *initialData, GpuBuffer *buffer) {
	(void)desc;
	(void)initialData;
	(void)buffer;
	return false;
}


bool ou::GraphicsDevice_DX12::CreateTexture(const TextureDesc *desc, const SubresourceData *initialData, Texture *texture) {
	(void)desc;
	(void)initialData;
	(void)texture;
	return false;
}


bool ou::GraphicsDevice_DX12::CreateShader(ShaderStage stage, const void *shaderCode, size_t shaderCodeSize, Shader *shader) {
	(void)stage;
	(void)shaderCode;
	(void)shaderCodeSize;
	(void)shader;
	return false;
}


bool ou::GraphicsDevice_DX12::CreateSampler(const SamplerDesc *desc, Sampler *sampler) {
	(void)desc;
	(void)sampler;
	return false;
}


bool ou::GraphicsDevice_DX12::CreatePipelineState(const PipelineStateDesc *desc, PipelineState *pso) {
	(void)desc;
	(void)pso;
	return false;
}


void ou::GraphicsDevice_DX12::SetName(GpuResource *resource, std::string_view name) const {
	(void)resource;
	(void)name;
}


CommandList ou::GraphicsDevice_DX12::BeginCommandList(QueueType queue) {
	(void)queue;
	return CommandList();
}


void ou::GraphicsDevice_DX12::SubmitCommandLists() {
}


void ou::GraphicsDevice_DX12::WaitForGpu() const {
}


void ou::GraphicsDevice_DX12::RenderPassBegin(const SwapChain *swapChain, CommandList cmd) {
	(void)swapChain;
	(void)cmd;
}


void ou::GraphicsDevice_DX12::RenderPassBegin(const RenderPassImage *images, uint32_t imageCount, CommandList cmd) {
	(void)images;
	(void)imageCount;
	(void)cmd;
}


void ou::GraphicsDevice_DX12::RenderPassEnd(CommandList cmd) {
	(void)cmd;
}


void ou::GraphicsDevice_DX12::BindViewports(uint32_t numViewports, const Viewport *viewports, CommandList cmd) {
	(void)numViewports;
	(void)viewports;
	(void)cmd;
}


void ou::GraphicsDevice_DX12::BindScissorRects(uint32_t numScissors, const Scissor *scissors, CommandList cmd) {
	(void)numScissors;
	(void)scissors;
	(void)cmd;
}


void ou::GraphicsDevice_DX12::BindPipelineState(const PipelineState *pso, CommandList cmd) {
	(void)pso;
	(void)cmd;
}


void ou::GraphicsDevice_DX12::BindVertexBuffers(const GpuBuffer *const *buffers, uint32_t slot, uint32_t count, const uint32_t *strides, const uint64_t *offsets, CommandList cmd) {
	(void)buffers;
	(void)slot;
	(void)count;
	(void)strides;
	(void)offsets;
	(void)cmd;
}


void ou::GraphicsDevice_DX12::BindIndexBuffer(const GpuBuffer *buffer, IndexBufferFormat format, uint64_t offsets, CommandList cmd) {
	(void)buffer;
	(void)format;
	(void)offsets;
	(void)cmd;
}


void ou::GraphicsDevice_DX12::BindConstantBuffer(const GpuBuffer *buffer, uint32_t slot, CommandList cmd, uint64_t offset) {
	(void)buffer;
	(void)slot;
	(void)cmd;
	(void)offset;
}


void ou::GraphicsDevice_DX12::BindResource(const GpuResource *resource, uint32_t slot, CommandList cmd, int subresource) {
	(void)resource;
	(void)slot;
	(void)cmd;
	(void)subresource;
}


void ou::GraphicsDevice_DX12::BindSampler(const Sampler *sampler, uint32_t slot, CommandList cmd) {
	(void)sampler;
	(void)slot;
	(void)cmd;
}


void ou::GraphicsDevice_DX12::Draw(uint32_t vertexCount, uint32_t startVertexLocation, CommandList cmd) {
	(void)vertexCount;
	(void)startVertexLocation;
	(void)cmd;
}


void ou::GraphicsDevice_DX12::DrawIndexed(uint32_t indexCount, uint32_t startIndexLocation, int32_t baseVertexLocation, CommandList cmd) {
	(void)indexCount;
	(void)startIndexLocation;
	(void)baseVertexLocation;
	(void)cmd;
}


void ou::GraphicsDevice_DX12::DrawInstanced(uint32_t vertexCount, uint32_t instanceCount, uint32_t startVertexLocation, uint32_t startInstanceLocation, CommandList cmd) {
	(void)vertexCount;
	(void)instanceCount;
	(void)startInstanceLocation;
	(void)startVertexLocation;
	(void)cmd;
}


void ou::GraphicsDevice_DX12::DrawIndexedInstanced(uint32_t indexCount, uint32_t instanceCount, uint32_t startIndexLocation, int32_t baseVertexLocation, uint32_t startInstanceLocation, CommandList cmd) {
	(void)indexCount;
	(void)instanceCount;
	(void)startIndexLocation;
	(void)baseVertexLocation;
	(void)startInstanceLocation;
	(void)cmd;
}


void ou::GraphicsDevice_DX12::Barrier(const GpuBarrier *barriers, uint32_t numBarriers, CommandList cmd) {
	(void)barriers;
	(void)numBarriers;
	(void)cmd;
}
