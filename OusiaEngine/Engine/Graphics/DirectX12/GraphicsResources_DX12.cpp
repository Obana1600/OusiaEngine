#include "GraphicsResources_DX12.h"
#include "Diagnostics/Fatal.h"
#include "Diagnostics/Log.h"
#include "Graphics/DirectX12/GraphicsConvert_DX12.h"

using namespace ou;
using namespace ou::Graphics;


ou::Graphics::SwapChain_DX12::~SwapChain_DX12() {
	if (!allocationHandler) { return; }

	// RTV はすぐ返してよい
	for (const auto &rtv : backBufferRtvs) {
		allocationHandler->descriptorRtv.Free(rtv);
	}

	// バックバッファは GPU が使い終わるまで解放を遅らせる
	std::scoped_lock lock(allocationHandler->destroyLocker);
	const auto frameCount = allocationHandler->frameCount;
	for (auto &backBuffer : backBuffers) {
		allocationHandler->destroyerResources.push_back({backBuffer, frameCount});
	}
}


ou::Graphics::Resource_DX12::~Resource_DX12() {
	if (!allocationHandler) { return; }

	// GPU が使い終わったら、リソースとバインドレス番号を手放す
	std::scoped_lock lock(allocationHandler->destroyLocker);
	const auto frameCount = allocationHandler->frameCount;
	if (resource) {
		allocationHandler->destroyerResources.push_back({resource, frameCount});
	}
	if (srvIndex >= 0) {
		allocationHandler->destroyBindlessRes.push_back({srvIndex, frameCount});
	}
}


ou::Graphics::Texture_DX12::~Texture_DX12() {
	if (!allocationHandler) { return; }

	// RTV / DSV はすぐ返してよい
	if (rtv.ptr != 0) { allocationHandler->descriptorRtv.Free(rtv); }
	if (dsv.ptr != 0) { allocationHandler->descriptorDsv.Free(dsv); }
}


ou::Graphics::Shader_DX12::~Shader_DX12() {
	if (!allocationHandler || !computePipeline) { return; }

	std::scoped_lock lock(allocationHandler->destroyLocker);
	allocationHandler->destroyerPipelines.push_back({computePipeline, allocationHandler->frameCount});
}


ou::Graphics::PipelineState_DX12::~PipelineState_DX12() {
	if (!allocationHandler) { return; }

	std::scoped_lock lock(allocationHandler->destroyLocker);
	for (const auto &variant : variants) {
		allocationHandler->destroyerPipelines.push_back(
			{variant.pipelineState, allocationHandler->frameCount});
	}
}


SwapChain_DX12 *ou::Graphics::ToInternal(const SwapChain *param) {
	return static_cast<SwapChain_DX12 *>(param->internalState.get());
}


Resource_DX12 *ou::Graphics::ToInternal(const GpuResource *param) {
	return static_cast<Resource_DX12 *>(param->internalState.get());
}


Texture_DX12 *ou::Graphics::ToInternal(const Texture *param) {
	return static_cast<Texture_DX12 *>(param->internalState.get());
}


Shader_DX12 *ou::Graphics::ToInternal(const Shader *param) {
	return static_cast<Shader_DX12 *>(param->internalState.get());
}


PipelineState_DX12 *ou::Graphics::ToInternal(const PipelineState *param) {
	return static_cast<PipelineState_DX12 *>(param->internalState.get());
}


ID3D12PipelineState *ou::Graphics::FindOrCreatePipeline(ID3D12Device *device,
														PipelineState_DX12 &is,
														const RenderPassInfo &renderPassInfo) {
	std::scoped_lock lock(is.variantsLocker);

	for (const auto &variant : is.variants) {
		if (variant.renderPassInfo == renderPassInfo) {
			return variant.pipelineState.Get();
		}
	}
	auto desc = is.desc;
	desc.NumRenderTargets = renderPassInfo.rtCount;
	for (uint32_t i = 0; i < renderPassInfo.rtCount; i++) {
		desc.RTVFormats[i] = ConvertFormat(renderPassInfo.rtFormats[i]);
	}
	desc.DSVFormat = ConvertFormat(renderPassInfo.dsFormat);
	if (renderPassInfo.dsFormat == Format::UNKNOWN) { desc.DepthStencilState.DepthEnable = FALSE; }
	desc.SampleDesc.Count = renderPassInfo.sampleCount;
	desc.SampleDesc.Quality = 0;

	Microsoft::WRL::ComPtr<ID3D12PipelineState> pipelineState;
	auto hr = device->CreateGraphicsPipelineState(&desc, IID_PPV_ARGS(&pipelineState));
	if (FAILED(hr)) {
		Log::Error("PipelineState の作成に失敗しました: {}", DescribeHResult(hr));
		return nullptr;
	}

	is.variants.push_back({renderPassInfo, pipelineState});
	return pipelineState.Get();
}
