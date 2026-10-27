#include "GraphicRaycaster.h"
#include "Platform/Screen.h"
#include "Scene/GameObject.h"
#include "Scene/Transform.h"
#include "UI/Graphic.h"
#include "UI/RectTransform.h"

#include <algorithm>
#include <cstdint>

using namespace ou;


const std::vector<GraphicRaycaster *> &ou::GraphicRaycaster::GetRaycasters() {
	return sRaycasters_;
}


void ou::GraphicRaycaster::OnEnable() {
	sRaycasters_.push_back(this);
}


void ou::GraphicRaycaster::OnDisable() {
	std::erase(sRaycasters_, this);
}


void ou::GraphicRaycaster::Raycast(const PointerEventData &eventData, std::vector<RaycastResult> &resultAppendList) {
	std::vector<Graphic *> graphics;
	CollectGraphics(GetTransform(), graphics);

	const Vector2 canvasPoint = {eventData.position.x - static_cast<float>(Screen::GetWidth()) * 0.5f,
								 eventData.position.y - static_cast<float>(Screen::GetHeight()) * 0.5f};

	for (auto i = graphics.size(); i-- > 0;) {
		if (!ContainsCanvasPoint(graphics[i], canvasPoint)) { continue; }

		resultAppendList.push_back(RaycastResult{.gameObject = graphics[i]->GetGameObject(),
												 .module = this,
												 .depth = static_cast<int32_t>(i)});
	}
}


void ou::GraphicRaycaster::CollectGraphics(const Transform *transform, std::vector<Graphic *> &graphics) {
	auto *graphic = transform->GetGameObject()->GetComponent<Graphic>();
	if (graphic && graphic->IsActiveAndEnabled() && graphic->GetRaycastTarget()) {
		graphics.push_back(graphic);
	}

	for (const auto *child : transform->GetChildren()) {
		CollectGraphics(child, graphics);
	}
}


bool ou::GraphicRaycaster::ContainsCanvasPoint(const Graphic *graphic, const Vector2 &canvasPoint) {
	const auto *rectTransform = dynamic_cast<const RectTransform *>(graphic->GetTransform());
	if (!rectTransform) { return false; }

	const auto worldToLocal = rectTransform->GetLocalToWorldMatrix().Inverse();
	const auto local = worldToLocal.MultiplyPoint({canvasPoint, 0.0f});

	const auto &rect = rectTransform->GetRect();
	return local.x >= rect.center.x && local.x <= rect.center.x + rect.size.x &&
		   local.y >= rect.center.y && local.y <= rect.center.y + rect.size.y;
}
