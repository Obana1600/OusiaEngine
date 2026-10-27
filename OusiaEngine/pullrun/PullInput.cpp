#include "PullInput.h"
#include "Input/Input.h"
#include "Math/Mathf.h"
#include "Platform/Screen.h"
#include "Tuning.h"
#include "UI/EventSystem/EventSystem.h"

#include <algorithm>

using namespace ou;


namespace {

constexpr int kLeftButton = 0;
constexpr int kRightButton = 1;

} // namespace



void ou::PullInput::Update(float unscaledDeltaTime, bool canStart) {
	isFired_ = false;

	// ドラッグ中はマウスを優先する
	if (isDragging_ || Input::GetMouseButtonDown(kLeftButton)) {
		UpdateMouse(canStart);
		return;
	}
	UpdateGamepad(unscaledDeltaTime, canStart);
}


void ou::PullInput::Cancel() {
	isCharging_ = false;
	isFired_ = false;
	isDragging_ = false;
	chargeRatio_ = 0.0f;
	chargeTime_ = 0.0f;
}


void ou::PullInput::UpdateMouse(bool canStart) {
	if (Input::GetMouseButtonDown(kLeftButton)) {
		// メニューのボタンなど、UI の上で押したときは引っ張らない
		const auto *eventSystem = EventSystem::GetCurrent();
		if (eventSystem && eventSystem->IsPointerOverGameObject()) { return; }
		isDragging_ = true;
		dragAnchor_ = Input::GetMousePosition();
	}

	if (Input::GetMouseButtonDown(kRightButton)) {
		Cancel();
		return;
	}

	// 画面の座標は y が下向きなので、ワールドの向きに直す
	const auto screenDrag = Input::GetMousePosition() - dragAnchor_;
	const Vector2 drag = {screenDrag.x, -screenDrag.y};
	const auto length = drag.Length();

	const auto screenHeight = static_cast<float>(Screen::GetHeight());
	const auto isPulled = length >= screenHeight * Tuning::kDragDeadZoneScreenRatio;
	if (isPulled) { pullDirection_ = drag / length; }

	isCharging_ = canStart && isPulled;
	chargeRatio_ = isCharging_ ? Mathf::Clamp01(length / (screenHeight * Tuning::kMaxDragScreenRatio)) : 0.0f;

	if (Input::GetMouseButtonUp(kLeftButton) || !Input::GetMouseButton(kLeftButton)) {
		// 溜めていれば発射し、引きが短い・発射できない間なら何もしない
		const auto shouldFire = isCharging_;
		const auto ratio = chargeRatio_;
		Cancel();
		isFired_ = shouldFire;
		chargeRatio_ = ratio;
	}
}


void ou::PullInput::UpdateGamepad(float unscaledDeltaTime, bool canStart) {
	if (!Input::IsGamepadConnected()) {
		Cancel();
		return;
	}

	auto stick = Input::GetGamepadLeftStick();
	const auto amount = stick.Length();
	if (amount > 1.0f) { stick /= amount; }

	// スティックが中心へ戻る途中の値は拾わない
	if (amount >= Tuning::kInputDirectionThreshold) { pullDirection_ = stick / amount; }

	if (isCharging_) {
		if (amount < Tuning::kInputReleaseThreshold) {
			// 発射せずに離したら取り消し
			Cancel();
			return;
		}
		chargeTime_ = std::min(chargeTime_ + unscaledDeltaTime, Tuning::kMaxChargeTime);
		chargeRatio_ = chargeTime_ / Tuning::kMaxChargeTime;
		if (Input::GetGamepadButtonDown(GamepadButton::South)) {
			isCharging_ = false;
			isFired_ = true;
		}
		return;
	}

	// 向きが決まるだけ倒していれば溜めを始める
	if (canStart && amount >= Tuning::kInputDirectionThreshold) {
		isCharging_ = true;
		chargeTime_ = 0.0f;
		chargeRatio_ = 0.0f;
	}
}
