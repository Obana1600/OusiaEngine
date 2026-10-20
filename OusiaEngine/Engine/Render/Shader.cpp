#include "Shader.h"
#include "Core/Assert.h"
#include "Core/Log.h"
#include "Graphics/GraphicsDevice.h"

#include <utility>

using namespace ou;


namespace {

void ApplyOverride(
	const Shader::RenderStateOverride &override,
	Graphics::BlendState &blend,
	Graphics::DepthStencilState &depthStencil,
	Graphics::RasterizerState &rasterizer) {
	auto &target = blend.renderTarget[0];
	if (override.srcBlend) { target.srcBlend = *override.srcBlend; }
	if (override.dstBlend) { target.destBlend = *override.dstBlend; }
	if (override.blendOp) { target.blendOp = *override.blendOp; }
	target.blendEnable = !(target.srcBlend == Graphics::Blend::ONE &&
						   target.destBlend == Graphics::Blend::ZERO &&
						   target.blendOp == Graphics::BlendOp::ADD);

	if (override.zWrite) {
		depthStencil.depthWriteMask = *override.zWrite ? Graphics::DepthWriteMask::ALL
													   : Graphics::DepthWriteMask::ZERO;
	}
	if (override.cull) { rasterizer.cullMode = *override.cull; }
}

const Graphics::Shader *OptionalStage(const Graphics::Shader &shader) {
	return shader.IsValid() ? &shader : nullptr;
}

} // namespace



ou::Shader::Shader(std::vector<Pass> passes, int renderQueue)
	: passes_(std::move(passes)),
	  renderQueue_(renderQueue),
	  pipelineCaches_(passes_.size()) {}


const Graphics::PipelineState *ou::Shader::GetPipelineState(
	size_t passIndex,
	const RenderStateOverride &renderStateOverride,
	const Graphics::InputLayout *inputLayout,
	Graphics::PrimitiveTopology topology) const {
	ASSERT(passIndex < passes_.size());

	// キャッシュを探す
	auto &cache = pipelineCaches_[passIndex];
	for (const auto &cached : cache) {
		if (cached.renderStateOverride == renderStateOverride &&
			cached.inputLayout == inputLayout &&
			cached.topology == topology) {
			return &cached.pipelineState;
		}
	}

	// Pass を上書きする
	const auto &pass = passes_[passIndex];
	auto blend = pass.blend;
	auto depthStencil = pass.depthStencil;
	auto rasterizer = pass.rasterizer;
	ApplyOverride(renderStateOverride, blend, depthStencil, rasterizer);

	// パイプラインの設定
	Graphics::PipelineStateDesc desc{};
	desc.vs = OptionalStage(pass.vertexShader);
	desc.ps = OptionalStage(pass.pixelShader);
	desc.hs = OptionalStage(pass.hullShader);
	desc.ds = OptionalStage(pass.domainShader);
	desc.gs = OptionalStage(pass.geometryShader);
	desc.bs = &blend;
	desc.dss = &depthStencil;
	desc.rs = &rasterizer;
	desc.il = inputLayout;
	desc.pt = topology;

	// パイプラインの作成
	auto &cached = cache.emplace_back();
	cached.renderStateOverride = renderStateOverride;
	cached.inputLayout = inputLayout;
	cached.topology = topology;
	if (!Graphics::GetDevice()->CreatePipelineState(&desc, &cached.pipelineState)) {
		Log::Error("PSO の作成に失敗しました: {} の Pass {}", GetName(), passIndex);
		cache.pop_back();
		return nullptr;
	}

	return &cached.pipelineState;
}


const std::vector<Shader::Pass> &ou::Shader::GetPasses() const {
	return passes_;
}


size_t ou::Shader::GetPassCount() const {
	return passes_.size();
}


int ou::Shader::GetRenderQueue() const {
	return renderQueue_;
}
