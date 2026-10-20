#include "Graphic.h"
#include "Asset/AssetDatabase.h"
#include "Core/Fatal.h"
#include "Graphics/GraphicsDevice.h"
#include "Render/Shader.h"
#include "Scene/GameObject.h"
#include "UI/RectTransform.h"

#include <utility>

using namespace ou;


void ou::Graphic::StaticInitialize() {
	const auto shader = AssetDatabase::LoadAssetAtPath<Shader>("EngineAssets/Shader/Sprite.shader");
	if (!shader) {
		FatalError("UI の既定のシェーダの読み込みに失敗しました");
	}
	sDefaultMaterial_ = Material::Create(shader);
	sDefaultMaterial_->SetName("Default UI Material");

	sWhiteTexture_ = AssetDatabase::LoadAssetAtPath<Texture>("EngineAssets/Texture/white1x1.png");
	if (!sWhiteTexture_) {
		FatalError("UI の既定のテクスチャの読み込みに失敗しました");
	}
}


void ou::Graphic::StaticFinalize() {
	sDefaultMaterial_ = nullptr;
	sWhiteTexture_ = nullptr;
}


const std::shared_ptr<Material> &ou::Graphic::GetDefaultGraphicMaterial() {
	return sDefaultMaterial_;
}


void ou::Graphic::Awake() {
	rectTransform_ = dynamic_cast<RectTransform *>(GetTransform());

	// RectTransform でなければ差し替える
	if (!rectTransform_) {
		rectTransform_ = GetGameObject()->AddComponent<RectTransform>();
	}
}


void ou::Graphic::SetMaterial(std::shared_ptr<Material> material) {
	material_ = std::move(material);
}


const std::shared_ptr<Material> &ou::Graphic::GetMaterial() const {
	return material_ ? material_ : sDefaultMaterial_;
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


void ou::Graphic::DrawWithMaterial(Graphics::CommandList cmd, uint32_t vertexCount) const {
	const auto &material = GetMaterial();
	if (!material || !material->GetShader()) { return; }

	auto *device = Graphics::GetDevice();
	const auto &shader = material->GetShader();
	for (size_t pass = 0; pass < shader->GetPassCount(); pass++) {
		const auto *pso = shader->GetPipelineState(
			pass, material->GetRenderStateOverride(), nullptr, Graphics::PrimitiveTopology::TRIANGLESTRIP);
		if (!pso) { continue; }

		device->BindPipelineState(pso, cmd);
		device->Draw(vertexCount, 0, cmd);
	}
}
