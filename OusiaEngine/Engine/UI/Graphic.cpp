#include "Graphic.h"
#include "Asset/AssetDatabase.h"
#include "Core/Time.h"
#include "Diagnostics/Fatal.h"
#include "Render/Asset/Shader.h"
#include "Scene/GameObject.h"
#include "UI/CanvasRenderer.h"
#include "UI/CanvasUpdateRegistry.h"
#include "UI/EventSystem/EventSystem.h"
#include "UI/RectTransform.h"
#include "UI/VertexHelper.h"

#include <algorithm>
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

	canvasRenderer_ = GetGameObject()->GetComponent<CanvasRenderer>();
	if (!canvasRenderer_) {
		canvasRenderer_ = GetGameObject()->AddComponent<CanvasRenderer>();
	}
}


void ou::Graphic::Update() {
	auto &tween = colorTween_;
	if (tween.elapsed >= tween.duration) { return;}

	tween.elapsed += tween.ignoreTimeScale ? Time::GetUnscaledDeltaTime() : Time::GetDeltaTime();
	const auto t = std::min(tween.elapsed / tween.duration, 1.0f);
	canvasRenderer_->SetColor(Color::Lerp(tween.startColor, tween.targetColor, t));
}


void ou::Graphic::OnEnable() {
	SetAllDirty();
}


void ou::Graphic::OnDisable() {
	CanvasUpdateRegistry::UnRegisterCanvasElementForRebuild(this);
	canvasRenderer_->Clear();
}


void ou::Graphic::OnDestroy() {
	if (auto *eventSystem = EventSystem::GetCurrent()) {
		eventSystem->OnGameObjectDestroyed(GetGameObject());
	}
}


const std::shared_ptr<Material> &ou::Graphic::GetDefaultMaterial() const {
	return sDefaultMaterial_;
}


const std::shared_ptr<Texture> &ou::Graphic::GetMainTexture() const {
	return sWhiteTexture_;
}


void ou::Graphic::SetMaterial(std::shared_ptr<Material> material) {
	material_ = std::move(material);
	SetMaterialDirty();
}


const std::shared_ptr<Material> &ou::Graphic::GetMaterial() const {
	return material_ ? material_ : GetDefaultMaterial();
}


void ou::Graphic::SetColor(const Color &color) {
	if (color_ == color) { return; }
	color_ = color;
	SetVerticesDirty();
}


const Color &ou::Graphic::GetColor() const {
	return color_;
}


void ou::Graphic::SetRaycastTarget(bool raycastTarget) {
	raycastTarget_ = raycastTarget;
}


bool ou::Graphic::GetRaycastTarget() const {
	return raycastTarget_;
}


void ou::Graphic::CrossFadeColor(const Color &targetColor, float duration, bool ignoreTimeScale, bool useAlpha) {
	const auto &currentColor = canvasRenderer_->GetColor();
	auto target = targetColor;
	if (!useAlpha) { target.a = currentColor.a; }

	if (duration <= 0.0f) {
		canvasRenderer_->SetColor(target);
		colorTween_ = {};
		return;
	}

	colorTween_ = {
		.startColor = currentColor,
		.targetColor = target,
		.duration = duration,
		.elapsed = 0.0f,
		.ignoreTimeScale = ignoreTimeScale};
}


CanvasRenderer *ou::Graphic::GetCanvasRenderer() const {
	return canvasRenderer_;
}


void ou::Graphic::SetAllDirty() {
	SetVerticesDirty();
	SetMaterialDirty();
}


void ou::Graphic::SetVerticesDirty() {
	if (!IsActiveAndEnabled() || !canvasRenderer_) { return; }

	isVerticesDirty_ = true;
	CanvasUpdateRegistry::RegisterCanvasElementForGraphicRebuild(this);
}


void ou::Graphic::SetMaterialDirty() {
	if (!IsActiveAndEnabled() || !canvasRenderer_) { return; }

	isMaterialDirty_ = true;
	CanvasUpdateRegistry::RegisterCanvasElementForGraphicRebuild(this);
}


void ou::Graphic::Rebuild() {
	if (isVerticesDirty_) {
		UpdateGeometry();
		isVerticesDirty_ = false;
	}
	if (isMaterialDirty_) {
		UpdateMaterial();
		isMaterialDirty_ = false;
	}
}


void ou::Graphic::OnRectTransformDimensionsChange() {
	SetVerticesDirty();
}


const std::shared_ptr<Texture> &ou::Graphic::GetWhiteTexture() {
	return sWhiteTexture_;
}


RectTransform *ou::Graphic::GetRectTransform() const {
	return rectTransform_;
}


void ou::Graphic::UpdateGeometry() {
	VertexHelper vh;
	OnPopulateMesh(vh);

	std::vector<UIVertex> vertices;
	std::vector<uint32_t> indices;
	vh.FillMesh(vertices, indices);
	canvasRenderer_->SetMesh(std::move(vertices), std::move(indices));
}


void ou::Graphic::UpdateMaterial() {
	canvasRenderer_->SetMaterialCount(1);
	canvasRenderer_->SetMaterial(GetMaterial(), 0);
	canvasRenderer_->SetTexture(GetMainTexture());
}


void ou::Graphic::OnPopulateMesh(VertexHelper &vh) {
	const auto &rect = rectTransform_->GetRect();
	const auto left = rect.center.x;
	const auto bottom = rect.center.y;
	const auto right = left + rect.size.x;
	const auto top = bottom + rect.size.y;

	vh.Clear();
	vh.AddVert({left, bottom, 0.0f}, color_, {0.0f, 1.0f, 0.0f, 0.0f});
	vh.AddVert({left, top, 0.0f}, color_, {0.0f, 0.0f, 0.0f, 0.0f});
	vh.AddVert({right, top, 0.0f}, color_, {1.0f, 0.0f, 0.0f, 0.0f});
	vh.AddVert({right, bottom, 0.0f}, color_, {1.0f, 1.0f, 0.0f, 0.0f});
	vh.AddTriangle(0, 1, 2);
	vh.AddTriangle(2, 3, 0);
}
