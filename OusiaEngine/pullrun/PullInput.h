#pragma once

#include "Math/Vector2.h"


namespace ou {

// 引っ張り（溜め）と発射の入力。
// マウス：左クリックした点からドラッグした距離が力になり、離すと反対向きに発射する。右クリックで取り消し。
// パッド：左スティックを倒している時間が力になり、A で発射する。スティックを離すと取り消し
class PullInput {
public: // メンバ関数

	// canStart が false の間は溜めを始めない。押し続けていれば、true になった瞬間から溜めを始める（先行入力）
	void Update(float unscaledDeltaTime, bool canStart);

	// 今回の Update で発射の入力があったか
	bool IsFired() const { return isFired_; }
	bool IsCharging() const { return isCharging_; }

	// 0〜1。マウスは最大の引きで、パッドは最大溜め時間で 1
	float GetChargeRatio() const { return chargeRatio_; }

	// 引っ張っている向き（入力量が閾値以上だった最後の向き）。発射はこの反対向き
	const Vector2 &GetPullDirection() const { return pullDirection_; }

	void Cancel();


private: // メンバ変数

	bool isCharging_ = false;
	bool isFired_ = false;
	float chargeRatio_ = 0.0f;
	Vector2 pullDirection_ = {0.0f, -1.0f};

	// マウス
	bool isDragging_ = false;
	Vector2 dragAnchor_;

	// パッド
	float chargeTime_ = 0.0f;


private: // メンバ関数

	void UpdateMouse(bool canStart);
	void UpdateGamepad(float unscaledDeltaTime, bool canStart);
};

} // namespace ou
