#include "Canvas.h"
#include "Core/Screen.h"
#include "Scene/GameObject.h"
#include "Scene/Scene.h"
#include "Scene/Transform.h"
#include "UI/Graphic.h"
#include "UI/RectTransform.h"

using namespace ou;


void ou::Canvas::Awake() {
	rectTransform_ = dynamic_cast<RectTransform *>(GetTransform());
	if (!rectTransform_) {
		rectTransform_ = GetGameObject()->AddComponent<RectTransform>();
	}
	FitToScreen();

	GetGameObject()->GetScene()->RegisterCanvas(this);
}


void ou::Canvas::LateUpdate() {
	FitToScreen();
}


void ou::Canvas::OnDestroy() {
	GetGameObject()->GetScene()->UnregisterCanvas(this);
}


void ou::Canvas::Render(Graphics::CommandList cmd) const {
	const auto &size = rectTransform_->GetSizeDelta();
	const auto viewProj = Matrix4x4::Ortho(size.x, size.y, 0.0f, 1.0f);

	std::vector<const Graphic *> graphics;
	CollectGraphics(rectTransform_, graphics);
	for (const auto *graphic : graphics) {
		graphic->Draw(cmd, viewProj);
	}
}


void ou::Canvas::FitToScreen() {
	const Vector2 screenSize = {
		static_cast<float>(Screen::GetWidth()),
		static_cast<float>(Screen::GetHeight())};
	if (rectTransform_->GetSizeDelta() == screenSize) { return; }

	rectTransform_->SetPivot({0.5f, 0.5f});
	rectTransform_->SetAnchoredPosition({});
	rectTransform_->SetSizeDelta(screenSize);
}


void ou::Canvas::CollectGraphics(const Transform *transform, std::vector<const Graphic *> &graphics) {
	if (const auto *graphic = transform->GetGameObject()->GetComponent<Graphic>()) {
		graphics.push_back(graphic);
	}

	// 子の Graphic も同じように集める
	for (const auto *child : transform->GetChildren()) {
		CollectGraphics(child, graphics);
	}
}
