#include "Material.h"
#include "Asset/AssetDatabase.h"
#include "Core/Fatal.h"
#include "Graphics/GraphicsDevice.h"
#include "Render/Mesh.h"

#include <utility>

using namespace ou;


void ou::Material::StaticInitialize(const Graphics::RenderPassInfo &renderPass) {
	// シェーダの作成
	sVs_ = AssetDatabase::LoadAssetAtPath<Shader>("EngineAssets/Shader/Standard.VS.hlsl");
	sPs_ = AssetDatabase::LoadAssetAtPath<Shader>("EngineAssets/Shader/Standard.PS.hlsl");
	if (!sVs_ || !sPs_) {
		FatalError("マテリアルのシェーダの作成に失敗しました");
	}

	// 既定のテクスチャ
	sWhiteTexture_ = AssetDatabase::LoadAssetAtPath<Texture>("EngineAssets/Texture/white1x1.png");
	if (!sWhiteTexture_) {
		FatalError("マテリアルの既定のテクスチャの読み込みに失敗しました");
	}

	// インプットレイアウト
	const Graphics::InputLayout layout = Mesh::MakeInputLayout();

	// ラスタライザ
	Graphics::RasterizerState rasterizer{};
	rasterizer.cullMode = Graphics::CullMode::BACK;

	// 深度ステンシル
	Graphics::DepthStencilState depthStencil{};
	depthStencil.depthEnable = true;
	depthStencil.depthWriteMask = Graphics::DepthWriteMask::ALL;
	depthStencil.comparisonFunc = Graphics::ComparisonFunc::LESS;

	// PSO の作成
	Graphics::PipelineStateDesc desc{};
	desc.vs = &sVs_->GetGraphicsShader();
	desc.ps = &sPs_->GetGraphicsShader();
	desc.il = &layout;
	desc.rs = &rasterizer;
	desc.dss = &depthStencil;
	desc.pt = Graphics::PrimitiveTopology::TRIANGLELIST;
	if (!Graphics::GetDevice()->CreatePipelineState(&desc, &sPso_, &renderPass)) {
		FatalError("Standard の PSO の作成に失敗しました。");
	}
}


void ou::Material::StaticFinalize() {
	sPso_ = {};
	sVs_ = nullptr;
	sPs_ = nullptr;
	sWhiteTexture_ = nullptr;
}


void ou::Material::BindPipelineState(Graphics::CommandList cmd) {
	Graphics::GetDevice()->BindPipelineState(&sPso_, cmd);
}


std::shared_ptr<Material> ou::Material::Create() {
	auto material = std::make_shared<Material>();
	material->SetMainTexture(sWhiteTexture_);
	return material;
}


Material::PerMaterial ou::Material::MakePerMaterial() const {
	PerMaterial constants{};
	constants.uvTransform = Matrix4x4::SRT(
		{uvScale_, 1.0f},
		Vector3{0.0f, 0.0f, uvRotation_},
		{uvOffset_, 0.0f});
	constants.color = color_;
	constants.lightingMode = lightingMode_;

	const auto &texture = mainTexture_ ? mainTexture_ : sWhiteTexture_;
	constants.textureIndex_ = texture->GetDescriptorIndex();

	return constants;
}


void ou::Material::SetColor(const Vector4 &color) {
	color_ = color;
}


const Vector4 &ou::Material::GetColor() const {
	return color_;
}


void ou::Material::SetMainTexture(std::shared_ptr<Texture> mainTexture) {
	mainTexture_ = std::move(mainTexture);
}


const std::shared_ptr<Texture> &ou::Material::GetMainTexture() const {
	return mainTexture_;
}


void ou::Material::SetUvOffset(const Vector2 &uvOffset) {
	uvOffset_ = uvOffset;
}


const Vector2 &ou::Material::GetUvOffset() const {
	return uvOffset_;
}


void ou::Material::SetUvRotation(float uvRotation) {
	uvRotation_ = uvRotation;
}


float ou::Material::GetUvRotation() const {
	return uvRotation_;
}


void ou::Material::SetUvScale(const Vector2 &uvScale) {
	uvScale_ = uvScale;
}


const Vector2 &ou::Material::GetUvScale() const {
	return uvScale_;
}


void ou::Material::SetLightingMode(LightingMode lightingMode) {
	lightingMode_ = lightingMode;
}


Material::LightingMode ou::Material::GetLightingMode() const {
	return lightingMode_;
}
