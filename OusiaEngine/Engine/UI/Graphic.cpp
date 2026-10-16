#include "Graphic.h"
#include "Asset/AssetDatabase.h"
#include "Core/Fatal.h"
#include "Graphics/GraphicsDevice.h"
#include "Scene/GameObject.h"
#include "Scene/RectTransform.h"

using namespace ou;


void ou::Graphic::StaticInitialize(const Graphics::RenderPassInfo &renderPass) {
	sVs_ = AssetDatabase::LoadAssetAtPath<Shader>("EngineAssets/Shader/Sprite.VS.hlsl");
	sPs_ = AssetDatabase::LoadAssetAtPath<Shader>("EngineAssets/Shader/Sprite.PS.hlsl");
	if (!sVs_ || !sPs_) {
		FatalError("UI のシェーダの作成に失敗しました");
	}

	sWhiteTexture_ = AssetDatabase::LoadAssetAtPath<Texture>("EngineAssets/Texture/white1x1.png");
	if (!sWhiteTexture_) {
		FatalError("UI の既定のテクスチャの読み込みに失敗しました");
	}

	Graphics::BlendState blend{};
	blend.renderTarget[0].blendEnable = true;
	blend.renderTarget[0].destBlendAlpha = Graphics::Blend::ZERO;

	Graphics::PipelineStateDesc desc{};
	desc.vs = &sVs_->GetGraphicsShader();
	desc.ps = &sPs_->GetGraphicsShader();
	desc.bs = &blend;
	desc.pt = Graphics::PrimitiveTopology::TRIANGLESTRIP;
	if (!Graphics::GetDevice()->CreatePipelineState(&desc, &sPso_, &renderPass)) {
		FatalError("UI の PSO の作成に失敗しました");
	}
}


void ou::Graphic::StaticFinalize() {
	sPso_ = {};
	sWhiteTexture_ = nullptr;
	sVs_ = nullptr;
	sPs_ = nullptr;
}


void ou::Graphic::BindPipelineState(Graphics::CommandList cmd) {
	Graphics::GetDevice()->BindPipelineState(&sPso_, cmd);
}


void ou::Graphic::Awake() {
	rectTransform_ = dynamic_cast<RectTransform *>(GetTransform());

	// RectTransform でなければ差し替える
	if (!rectTransform_) {
		rectTransform_ = GetGameObject()->AddComponent<RectTransform>();
	}
}


void ou::Graphic::SetColor(const Color &color) {
	color_ = color;
}


const Color &ou::Graphic::GetColor() const {
	return color_;
}


const std::shared_ptr<Texture> &ou::Graphic::GetWhiteTexture() {
	return sWhiteTexture_;
}


RectTransform *ou::Graphic::GetRectTransform() const {
	return rectTransform_;
}
