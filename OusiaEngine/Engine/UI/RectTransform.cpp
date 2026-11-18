#include "RectTransform.h"
#include "Scene/GameObject.h"
#include "Serialize/Serializer.h"
#include "UI/Graphic.h"

using namespace ou;


void ou::RectTransform::Awake() {
	RecalculateLayout();
}


void ou::RectTransform::Serialize(Serializer &serializer) {
	Transform::Serialize(serializer);

	serializer.Field("anchorMin", anchorMin_);
	serializer.Field("anchorMax", anchorMax_);
	serializer.Field("anchoredPosition", anchoredPosition_);
	serializer.Field("sizeDelta", sizeDelta_);
	serializer.Field("pivot", pivot_);
}


void ou::RectTransform::AwakeFromLoad() {
	Transform::AwakeFromLoad();
	RecalculateLayout();
}


void ou::RectTransform::SetAnchorMin(const Vector2 &anchorMin) {
	anchorMin_ = anchorMin;
	RecalculateLayout();
}


const Vector2 &ou::RectTransform::GetAnchorMin() const {
	return anchorMin_;
}


void ou::RectTransform::SetAnchorMax(const Vector2 &anchorMax) {
	anchorMax_ = anchorMax;
	RecalculateLayout();
}


const Vector2 &ou::RectTransform::GetAnchorMax() const {
	return anchorMax_;
}


void ou::RectTransform::SetAnchoredPosition(const Vector2 &anchoredPosition) {
	anchoredPosition_ = anchoredPosition;
	RecalculateLayout();
}


const Vector2 &ou::RectTransform::GetAnchoredPosition() const {
	return anchoredPosition_;
}


void ou::RectTransform::SetSizeDelta(const Vector2 &sizeDelta) {
	sizeDelta_ = sizeDelta;
	RecalculateLayout();
}


const Vector2 &ou::RectTransform::GetSizeDelta() const {
	return sizeDelta_;
}


void ou::RectTransform::SetPivot(const Vector2 &pivot) {
	pivot_ = pivot;
	RecalculateLayout();
}


const Vector2 &ou::RectTransform::GetPivot() const {
	return pivot_;
}


const Rect &ou::RectTransform::GetRect() const {
	return rect_;
}


void ou::RectTransform::OnTransformParentChanged() {
	RecalculateLayout();
}


void ou::RectTransform::RecalculateLayout() {
	const auto parentRect = FindParentRect();

	// アンカーの位置
	const auto anchorMinPos = parentRect.GetMin() + Vector2::Scale(anchorMin_, parentRect.GetSize());
	const auto anchorMaxPos = parentRect.GetMin() + Vector2::Scale(anchorMax_, parentRect.GetSize());
	const auto anchorSpan = anchorMaxPos - anchorMinPos;

	// サイズとピボット
	const auto size = anchorSpan + sizeDelta_;
	const auto pivotPos = anchorMinPos + Vector2::Scale(anchorSpan, pivot_) + anchoredPosition_;

	const auto rect = Rect(-Vector2::Scale(pivot_, size), size);
	const auto isRectChanged = rect != rect_;
	rect_ = rect;
	SetLocalPosition({pivotPos.x, pivotPos.y, GetLocalPosition().z});

	// 子の再計算
	for (auto *child : GetChildren()) {
		if (auto *rectChild = dynamic_cast<RectTransform *>(child)) {
			rectChild->RecalculateLayout();
		}
	}

	if (isRectChanged) {
		if (auto *graphic = GetGameObject()->GetComponent<Graphic>()) {
			graphic->OnRectTransformDimensionsChange();
		}
	}
}


Rect ou::RectTransform::FindParentRect() const {
	if (const auto *parent = dynamic_cast<const RectTransform *>(GetParent())) {
		return parent->GetRect();
	}

	return {};
}
