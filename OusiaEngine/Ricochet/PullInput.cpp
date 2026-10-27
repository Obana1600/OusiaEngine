#include "PullInput.h"
#include "Input/Input.h"
#include "Math/Mathf.h"
#include "Tuning.h"

#include <algorithm>

using namespace ou;



void ou::PullInput::Update(float unscaledDeltaTime, bool canStart) {
	isFired_ = false;

	const auto raw = ReadPullInput();
	const auto amount = raw.Length();

	// スティックが中心へ戻る途中の値は拾わない
	if (amount >= Tuning::kInputDirectionThreshold) { pullDirection_ = raw / amount; }

	if (isCharging_) {
		if (amount < Tuning::kInputReleaseThreshold) {
			// 発射せずに離したら取り消し
			Cancel();
			return;
		}
		chargeTime_ = std::min(chargeTime_ + unscaledDeltaTime, Tuning::kMaxChargeTime);
		if (ReadFireInput()) {
			isCharging_ = false;
			isFired_ = true;
		}
		return;
	}

	// 向きが決まるだけ倒していれば溜めを始める
	if (canStart && amount >= Tuning::kInputDirectionThreshold) {
		isCharging_ = true;
		chargeTime_ = 0.0f;
	}
}


float ou::PullInput::GetChargeRatio() const {
	return Mathf::Clamp01(chargeTime_ / Tuning::kMaxChargeTime);
}


void ou::PullInput::Cancel() {
	isCharging_ = false;
	isFired_ = false;
	chargeTime_ = 0.0f;
}


Vector2 ou::PullInput::ReadPullInput() {
	Vector2 keyboard;
	if (Input::GetKey(KeyCode::D)) { keyboard.x += 1.0f; }
	if (Input::GetKey(KeyCode::A)) { keyboard.x -= 1.0f; }
	if (Input::GetKey(KeyCode::W)) { keyboard.y += 1.0f; }
	if (Input::GetKey(KeyCode::S)) { keyboard.y -= 1.0f; }
	if (keyboard.LengthSquared() > 0.0f) { return keyboard.Normalize(); }

	if (Input::IsGamepadConnected()) {
		auto stick = Input::GetGamepadLeftStick();
		const auto length = stick.Length();
		if (length > 1.0f) { stick /= length; }
		return stick;
	}
	return Vector2::Zero;
}


bool ou::PullInput::ReadFireInput() {
	if (Input::GetKeyDown(KeyCode::Space)) { return true; }
	return Input::IsGamepadConnected() && Input::GetGamepadButtonDown(GamepadButton::South);
}
