#include "XInput.h"
#include "Diagnostics/Log.h"
#include "Math/Mathf.h"

#include <algorithm>
#include <array>
#include <cmath>

#include <Windows.h>
#include <Xinput.h>

#pragma comment(lib, "xinput.lib")

using namespace ou;
using namespace ou::Input;


namespace {

// スティック / トリガーの最大値
constexpr float kMaxThumb = 32767.0f;
constexpr float kMaxTrigger = 255.0f;

// グローバルなインスタンス
std::array<XInput::GamepadState, XInput::kMaxGamepadCount> sGamepads{};
bool sNeedScan = true;


Vector2 ToStick(SHORT x, SHORT y, SHORT deadZone) {
	const Vector2 f = {static_cast<float>(x), static_cast<float>(y)};
	const auto length = f.Length();
	if (length <= deadZone) { return Vector2::Zero; }

	const auto scaled = Mathf::Min((length - deadZone) / (kMaxThumb - deadZone), 1.0f);
	return f / length * scaled;
}

float ToTrigger(BYTE value) {
	if (value <= XINPUT_GAMEPAD_TRIGGER_THRESHOLD) { return 0.0f; }
	return (value - XINPUT_GAMEPAD_TRIGGER_THRESHOLD) / (kMaxTrigger - XINPUT_GAMEPAD_TRIGGER_THRESHOLD);
}

void StopMotors() {
	for (size_t i = 0; i < XInput::kMaxGamepadCount; i++) {
		XInput::SetMotorSpeeds(i, 0.0f, 0.0f);
	}
}

} // namespace



void ou::Input::XInput::Finalize() {
	StopMotors();
	sGamepads = {};
	sNeedScan = true;
}


void ou::Input::XInput::Update() {
	for (DWORD i = 0; i < kMaxGamepadCount; i++) {
		auto &pad = sGamepads[i];
		if (!pad.isConnected && !sNeedScan) { continue; }

		XINPUT_STATE state{};
		if (XInputGetState(i, &state) != ERROR_SUCCESS) {
			if (pad.isConnected) { Log::Info("ゲームパッド {} が外れました", i); }
			pad = {};
			continue;
		}

		if (!pad.isConnected) { Log::Info("ゲームパッド {} が繋がりました", i); }

		const auto &gamepad = state.Gamepad;
		pad.isConnected = true;
		pad.buttons = gamepad.wButtons;
		pad.leftStick = ToStick(gamepad.sThumbLX, gamepad.sThumbLY, XINPUT_GAMEPAD_LEFT_THUMB_DEADZONE);
		pad.rightStick = ToStick(gamepad.sThumbRX, gamepad.sThumbRY, XINPUT_GAMEPAD_RIGHT_THUMB_DEADZONE);
		pad.leftTrigger = ToTrigger(gamepad.bLeftTrigger);
		pad.rightTrigger = ToTrigger(gamepad.bRightTrigger);
	}

	sNeedScan = false;
}


void ou::Input::XInput::RequestScan() {
	sNeedScan = true;
}


void ou::Input::XInput::ReleaseAll() {
	StopMotors();

	// 接続状態だけ残す
	for (auto &pad : sGamepads) {
		pad = {.isConnected = pad.isConnected};
	}
}


void ou::Input::XInput::SetMotorSpeeds(size_t index, float low, float high) {
	if (index >= kMaxGamepadCount || !sGamepads[index].isConnected) { return; }

	XINPUT_VIBRATION vibration{};
	vibration.wLeftMotorSpeed = static_cast<WORD>(Mathf::Clamp01(low) * 65535.0f);
	vibration.wRightMotorSpeed = static_cast<WORD>(Mathf::Clamp01(high) * 65535.0f);
	XInputSetState(static_cast<DWORD>(index), &vibration);
}


const XInput::GamepadState &ou::Input::XInput::GetState(size_t index) {
	return sGamepads[index];
}
