#include "CopyDepthPass.h"
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

struct CopyDepthConstants {
	uint32_t sourceIndex;
	uint32_t padding[3];
};

} // namespace



void ou::CopyDepthPass::StaticInitialize() {
	sVs = AssetDatabase::LoadAssetAtPath<ShaderModule>("EngineAssets/Shader/Blit.VS.hlsl");
	sPs = AssetDatabase::LoadAssetAtPath<ShaderModule>("EngineAssets/Shader/CopyDepth.PS.hlsl");
	if (!sVs || !sPs) { FatalError("CopyDepth のシェーダの作成に失敗しました"); }

	Graphics::PipelineStateDesc desc{};
	desc.vs = &sVs->GetGraphicsShader();
	desc.ps = &sPs->GetGraphicsShader();
	if (!Graphics::GetDevice()->CreatePipelineState(&desc, &sPso)) {
		FatalError("CopyDepth の PSO の作成に失敗しました");
	}
}


void ou::CopyDepthPass::StaticFinalize() {
	sPso = {};
	sPs.reset();
	sVs.reset();
}


void ou::CopyDepthPass::Render(const SceneRenderTarget &target, Graphics::CommandList cmd) {
	auto *device = Graphics::GetDevice();
	const auto &depth = target.GetDepth();

	const auto toRead = Graphics::GpuBarrier::Image(
		&depth, Graphics::ResourceState::DEPTHSTENCIL, Graphics::ResourceState::SHADER_RESOURCE);
	device->Barrier(&toRead, 1, cmd);

	target.BeginDepthCopyPass(cmd);
	device->BindPipelineState(&sPso, cmd);

	CopyDepthConstants constants{};
	constants.sourceIndex = static_cast<uint32_t>(device->GetDescriptorIndex(&depth, Graphics::ViewType::SRV));
	device->BindDynamicConstantBuffer(constants, 0, cmd);

	device->Draw(3, 0, cmd);
	device->RenderPassEnd(cmd);

	const auto toWrite = Graphics::GpuBarrier::Image(
		&depth, Graphics::ResourceState::SHADER_RESOURCE, Graphics::ResourceState::DEPTHSTENCIL);
	device->Barrier(&toWrite, 1, cmd);
}
