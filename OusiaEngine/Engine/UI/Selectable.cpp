#include "Selectable.h"
#include "Scene/GameObject.h"
#include "UI/EventSystem/EventSystem.h"
#include "UI/Graphic.h"
#include "UI/RectTransform.h"

#include <algorithm>
#include <cmath>

using namespace ou;


namespace {

Vector2 FindPointOnRectEdge(const Rect &rect, Vector2 dir) {
	const auto center = rect.center + rect.size * 0.5f;
	if (dir == Vector2::Zero) { return center; }

	dir = dir / std::max(std::abs(dir.x), std::abs(dir.y));
	return center + Vector2::Scale(rect.size, dir * 0.5f);
}

} // namespace



const std::vector<Selectable *> &ou::Selectable::GetAllSelectables() {
	return sSelectables_;
}


void ou::Selectable::Awake() {
	if (!targetGraphic_) {
		targetGraphic_ = GetGameObject()->GetComponent<Graphic>();
	}
}


void ou::Selectable::OnEnable() {
	sSelectables_.push_back(this);

	const auto *eventSystem = EventSystem::GetCurrent();
	hasSelection_ = eventSystem && eventSystem->GetCurrentSelectedGameObject() == GetGameObject();
	isPointerDown_ = false;
	DoStateTransition(GetCurrentSelectionState(), true);
}


void ou::Selectable::OnDisable() {
	std::erase(sSelectables_, this);

	isPointerInside_ = false;
	isPointerDown_ = false;
	hasSelection_ = false;
	DoStateTransition(GetCurrentSelectionState(), true);
}


void ou::Selectable::OnDestroy() {
	if (auto *eventSystem = EventSystem::GetCurrent()) {
		eventSystem->OnGameObjectDestroyed(GetGameObject());
	}
}


void ou::Selectable::SetInteractable(bool interactable) {
	if (interactable_ == interactable) { return; }

	interactable_ = interactable;

	auto *eventSystem = EventSystem::GetCurrent();
	if (!interactable_ && eventSystem && eventSystem->GetCurrentSelectedGameObject() == GetGameObject()) {
		eventSystem->SetSelectedGameObject(nullptr);
	}
	EvaluateAndTransitionToSelectionState();
}


bool ou::Selectable::IsInteractable() const {
	return interactable_;
}


void ou::Selectable::SetTargetGraphic(Graphic *targetGraphic) {
	targetGraphic_ = targetGraphic;
	DoStateTransition(GetCurrentSelectionState(), true);
}


Graphic *ou::Selectable::GetTargetGraphic() const {
	return targetGraphic_;
}


void ou::Selectable::SetColors(const ColorBlock &colors) {
	colors_ = colors;
	DoStateTransition(GetCurrentSelectionState(), true);
}


const ColorBlock &ou::Selectable::GetColors() const {
	return colors_;
}


Selectable *ou::Selectable::FindSelectable(const Vector3 &dir) const {
	const auto direction = dir.Normalized();

	auto pos = GetTransform()->GetWorldPosition();
	if (const auto *rectTransform = dynamic_cast<const RectTransform *>(GetTransform())) {
		const auto &localToWorld = rectTransform->GetLocalToWorldMatrix();
		const auto localDir = localToWorld.Inverse().MultiplyVector(direction);
		const auto edge = FindPointOnRectEdge(rectTransform->GetRect(), {localDir.x, localDir.y});
		pos = localToWorld.MultiplyPoint({edge.x, edge.y, 0.0f});
	}

	float maxScore = 0.0f;
	Selectable *bestPick = nullptr;
	for (auto *selectable : sSelectables_) {
		if (selectable == this || !selectable->IsInteractable()) { continue; }

		const auto toSelectable = selectable->CalculateWorldCenter() - pos;
		const auto dot = Vector3::Dot(direction, toSelectable);
		if (dot <= 0.0f) { continue; }

		const auto score = dot / toSelectable.LengthSquared();
		if (score > maxScore) {
			maxScore = score;
			bestPick = selectable;
		}
	}
	return bestPick;
}


Selectable *ou::Selectable::FindSelectableOnLeft() const {
	return FindSelectable(Vector3::Left);
}


Selectable *ou::Selectable::FindSelectableOnRight() const {
	return FindSelectable(Vector3::Right);
}


Selectable *ou::Selectable::FindSelectableOnUp() const {
	return FindSelectable(Vector3::Up);
}


Selectable *ou::Selectable::FindSelectableOnDown() const {
	return FindSelectable(Vector3::Down);
}


void ou::Selectable::Select() {
	if (auto *eventSystem = EventSystem::GetCurrent()) {
		eventSystem->SetSelectedGameObject(GetGameObject());
	}
}


void ou::Selectable::OnPointerEnter(PointerEventData &) {
	isPointerInside_ = true;
	EvaluateAndTransitionToSelectionState();

	if (IsInteractable()) {
		Select();
	}
}


void ou::Selectable::OnPointerExit(PointerEventData &) {
	isPointerInside_ = false;
	EvaluateAndTransitionToSelectionState();
}


void ou::Selectable::OnPointerDown(PointerEventData &eventData) {
	if (eventData.button != PointerEventData::InputButton::Left) { return; }

	if (IsInteractable()) {
		Select();
	}

	isPointerDown_ = true;
	EvaluateAndTransitionToSelectionState();
}


void ou::Selectable::OnPointerUp(PointerEventData &eventData) {
	if (eventData.button != PointerEventData::InputButton::Left) { return; }

	isPointerDown_ = false;
	EvaluateAndTransitionToSelectionState();
}


void ou::Selectable::OnSelect(BaseEventData &) {
	hasSelection_ = true;
	EvaluateAndTransitionToSelectionState();
}


void ou::Selectable::OnDeselect(BaseEventData &) {
	hasSelection_ = false;
	EvaluateAndTransitionToSelectionState();
}


void ou::Selectable::OnMove(AxisEventData &eventData) {
	Selectable *next = nullptr;
	switch (eventData.moveDir) {
		case MoveDirection::Left:
			next = FindSelectableOnLeft();
			break;
		case MoveDirection::Right:
			next = FindSelectableOnRight();
			break;
		case MoveDirection::Up:
			next = FindSelectableOnUp();
			break;
		case MoveDirection::Down:
			next = FindSelectableOnDown();
			break;
		case MoveDirection::None:
			break;
	}

	if (next) {
		next->Select();
	}
}


Selectable::SelectionState ou::Selectable::GetCurrentSelectionState() const {
	if (!IsInteractable()) { return SelectionState::Disabled; }
	if (isPointerDown_) { return SelectionState::Pressed; }
	if (hasSelection_) { return SelectionState::Selected; }
	if (isPointerInside_) { return SelectionState::Highlighted; }
	return SelectionState::Normal;
}


void ou::Selectable::DoStateTransition(SelectionState state, bool instant) {
	if (!targetGraphic_) { return; }

	auto color = colors_.normalColor;
	switch (state) {
		case SelectionState::Normal:
			color = colors_.normalColor;
			break;
		case SelectionState::Highlighted:
			color = colors_.highlightedColor;
			break;
		case SelectionState::Pressed:
			color = colors_.pressedColor;
			break;
		case SelectionState::Selected:
			color = colors_.selectedColor;
			break;
		case SelectionState::Disabled:
			color = colors_.disabledColor;
			break;
	}

	targetGraphic_->CrossFadeColor(
		color * colors_.colorMultiplier, instant ? 0.0f : colors_.fadeDuration, true, true);
}


void ou::Selectable::EvaluateAndTransitionToSelectionState() {
	if (!IsActiveAndEnabled()) { return; }

	DoStateTransition(GetCurrentSelectionState(), false);
}


Vector3 ou::Selectable::CalculateWorldCenter() const {
	const auto *rectTransform = dynamic_cast<const RectTransform *>(GetTransform());
	if (!rectTransform) { return GetTransform()->GetWorldPosition(); }

	const auto &rect = rectTransform->GetRect();
	const auto center = rect.center + rect.size * 0.5f;
	return rectTransform->GetLocalToWorldMatrix().MultiplyPoint({center, 0.0f});
}
