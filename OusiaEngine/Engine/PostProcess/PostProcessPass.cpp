#include "PostProcessPass.h"
#include "Asset/AssetDatabase.h"
#include "Diagnostics/Fatal.h"
#include "Editor/SceneRenderTarget.h"
#include "Graphics/GraphicsDevice.h"
#include "Render/Asset/ShaderModule.h"

#include <cstdint>
#include <memory>

using namespace ou;


namespace {

struct UberPostConstants {
	uint32_t sourceIndex;
};

std::shared_ptr<ShaderModule> sFullScreenVs;
std::shared_ptr<ShaderModule> sUberPostPs;
Graphics::PipelineState sUberPostPso;

} // namespace



void ou::PostProcessPass::StaticInitialize() {
	sFullScreenVs = AssetDatabase::LoadAssetAtPath<ShaderModule>("EngineAssets/Shader/Blit.VS.hlsl");
	sUberPostPs = AssetDatabase::LoadAssetAtPath<ShaderModule>("EngineAssets/Shader/UberPost.PS.hlsl");
	if (!sFullScreenVs || !sUberPostPs) {
		FatalError("UberPost のシェーダの作成に失敗しました");
	}

	Graphics::PipelineStateDesc desc{};
	desc.vs = &sFullScreenVs->GetGraphicsShader();
	desc.ps = &sUberPostPs->GetGraphicsShader();
	if (!Graphics::GetDevice()->CreatePipelineState(&desc, &sUberPostPso)) {
		FatalError("UberPost の PSO の作成に失敗しました");
	}
}


void ou::PostProcessPass::StaticFinalize() {
	sUberPostPso = {};
	sUberPostPs.reset();
	sFullScreenVs.reset();
}


void ou::PostProcessPass::Render(const SceneRenderTarget &target, Graphics::CommandList cmd) {
	auto *device = Graphics::GetDevice();

	target.BeginPostProcessPass(cmd);
	device->BindPipelineState(&sUberPostPso, cmd);

	UberPostConstants constants{};
	constants.sourceIndex = static_cast<uint32_t>(device->GetDescriptorIndex(&target.GetSceneColor(), Graphics::ViewType::SRV));
	device->BindDynamicConstantBuffer(constants, 0, cmd);

	device->Draw(3, 0, cmd);
	device->RenderPassEnd(cmd);
}
