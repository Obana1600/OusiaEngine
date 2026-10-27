#include "WarningLine.h"
#include "Render/LineRenderer.h"
#include "Scene/GameObject.h"
#include "Scene/Scene.h"

#include <vector>

using namespace ou;



ou::WarningLine::~WarningLine() {
	Destroy();
}


void ou::WarningLine::Create(Scene &scene, float width, const Color &color) {
	Destroy();
	line_ = scene.CreateGameObject("Warning Line")->AddComponent<LineRenderer>();
	line_->SetWidthMultiplier(width);
	color_ = color;
	SetAlpha(1.0f);
}


void ou::WarningLine::SetPoints(std::span<const Vector2> points) {
	if (!line_) { return; }
	std::vector<Vector3> positions;
	positions.reserve(points.size());
	// ボスや弾より奥に描いて、上に重ならないようにする
	for (const auto &point : points) { positions.push_back({point.x, point.y, 0.5f}); }
	line_->SetPositions(positions);
}


void ou::WarningLine::SetAlpha(float alpha) {
	if (!line_) { return; }
	auto color = color_;
	color.a *= alpha;
	line_->SetStartColor(color);
	color.a *= 0.3f;
	line_->SetEndColor(color);
}


void ou::WarningLine::Destroy() {
	if (!line_) { return; }
	Object::Destroy(line_->GetGameObject());
	line_ = nullptr;
}
