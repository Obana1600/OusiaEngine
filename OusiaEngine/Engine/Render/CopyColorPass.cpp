#include "CopyColorPass.h"
#include "Asset/AssetDatabase.h"
#include "Diagnostics/Fatal.h"
#include "Editor/SceneRenderTarget.h"
#include "Graphics/GraphicsDevice.h"
#include "Render/Asset/ShaderModule.h"

#include <cstdint>
#include <memory>

using namespace ou;


namespace {

std::shared_ptr<ShaderModule> sVs;
std::shared_ptr<ShaderModule> sPs;
Graphics::PipelineState sPso;

struct BlitConstants {
	uint32_t sourceIndex;
	uint32_t padding[3];
};

} // namespace



void ou::CopyColorPass::StaticInitialize() {
	sVs = AssetDatabase::LoadAssetAtPath<ShaderModule>("EngineAssets/Shader/Blit.VS.hlsl");
	sPs = AssetDatabase::LoadAssetAtPath<ShaderModule>("EngineAssets/Shader/Blit.PS.hlsl");
	if (!sVs || !sPs) { FatalError("CopyColor のシェーダの作成に失敗しました"); }

	Graphics::PipelineStateDesc desc{};
	desc.vs = &sVs->GetGraphicsShader();
	desc.ps = &sPs->GetGraphicsShader();
	if (!Graphics::GetDevice()->CreatePipelineState(&desc, &sPso)) {
		FatalError("CopyColor の PSO の作成に失敗しました");
	}
}


void ou::CopyColorPass::StaticFinalize() {
	sPso = {};
	sPs.reset();
	sVs.reset();
}


void ou::CopyColorPass::Render(const SceneRenderTarget &target, Graphics::CommandList cmd) {
	auto *device = Graphics::GetDevice();

	target.BeginOpaqueCopyPass(cmd);
	device->BindPipelineState(&sPso, cmd);

	BlitConstants constants{};
	constants.sourceIndex = static_cast<uint32_t>(device->GetDescriptorIndex(&target.GetSceneColor(), Graphics::ViewType::SRV));
	device->BindDynamicConstantBuffer(constants, 0, cmd);

	device->Draw(3, 0, cmd);
	device->RenderPassEnd(cmd);
}
