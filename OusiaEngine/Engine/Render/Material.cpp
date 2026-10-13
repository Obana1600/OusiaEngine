#include "Material.h"
#include "Core/Fatal.h"
#include "Graphics/DXCompiler.h"
#include "Graphics/GraphicsDevice.h"
#include "Graphics/TextureManager.h"
#include "Render/Mesh.h"

using namespace ou;


void ou::Material::StaticInitialize(const Graphics::RenderPassInfo &renderPass) {
	sVs_ = DXCompiler::GetInstance()->CompileEngineShader(L"Standard.VS.hlsl", Graphics::ShaderStage::VS);
	sPs_ = DXCompiler::GetInstance()->CompileEngineShader(L"Standard.PS.hlsl", Graphics::ShaderStage::PS);

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
	desc.vs = &sVs_;
	desc.ps = &sPs_;
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
	sPs_ = {};
	sVs_ = {};
}


void ou::Material::BindPipelineState(Graphics::CommandList cmd) {
	Graphics::GetDevice()->BindPipelineState(&sPso_, cmd);
}


std::shared_ptr<Material> ou::Material::Create() {
	auto material = std::make_shared<Material>();
	material->SetTextureHandle(TextureManager::Load("white1x1.png"));
	return material;
}


Material::PerMaterial ou::Material::MakePerMaterial() const {
	PerMaterial constants{};
	constants.uvTransform = Matrix4x4::SRT(
		{uvScale_, 1.0f},
		Vector3{0.0f, 0.0f, uvRotation_},
		{uvOffset_, 0.0f});
	constants.color = color_;
	constants.textureIndex_ = TextureManager::GetInstance()->GetDescriptorIndex(textureHandle_);
	constants.lightingMode = lightingMode_;

	return constants;
}


void ou::Material::SetColor(const Vector4 &color) {
	color_ = color;
}


const Vector4 &ou::Material::GetColor() const {
	return color_;
}


void ou::Material::SetTextureHandle(uint32_t textureHandle) {
	textureHandle_ = textureHandle;
}


uint32_t ou::Material::GetTextureHandle() const {
	return textureHandle_;
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
