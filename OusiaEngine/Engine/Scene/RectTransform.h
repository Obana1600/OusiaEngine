#pragma once

#include "Math/Rect.h"
#include "Math/Vector2.h"
#include "Scene/Transform.h"


namespace ou {

class RectTransform final: public Transform {
public: // メンバ関数

	void Awake() override;

	void SetAnchorMin(const Vector2 &anchorMin);
	const Vector2 &GetAnchorMin() const;

	void SetAnchorMax(const Vector2 &anchorMax);
	const Vector2 &GetAnchorMax() const;

	void SetAnchoredPosition(const Vector2 &anchoredPosition);
	const Vector2 &GetAnchoredPosition() const;

	void SetSizeDelta(const Vector2 &sizeDelta);
	const Vector2 &GetSizeDelta() const;

	void SetPivot(const Vector2 &pivot);
	const Vector2 &GetPivot() const;

	const Rect &GetRect() const;


protected: // メンバ関数

	void OnTransformParentChanged() override;


private: // メンバ変数

	Vector2 anchorMin_ = {0.5f, 0.5f};
	Vector2 anchorMax_ = {0.5f, 0.5f};
	Vector2 anchoredPosition_{};
	Vector2 sizeDelta_ = {100.0f, 100.0f};
	Vector2 pivot_ = {0.5f, 0.5f};

	Rect rect_{};


private: // メンバ関数

	void RecalculateLayout();

	Rect FindParentRect() const;
};

} // namespace ou
