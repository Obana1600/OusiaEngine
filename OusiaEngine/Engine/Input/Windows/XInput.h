#pragma once

#include "Math/Vector2.h"

#include <cstdint>


namespace ou::XInput {

// XInput がつなげられるパッドの数
constexpr size_t kMaxGamepadCount = 4;

struct GamepadState {
	bool isConnected = false;
	uint16_t buttons = 0;
	Vector2 leftStick{};
	Vector2 rightStick{};
	float leftTrigger = 0.0f;
	float rightTrigger = 0.0f;
};


void Finalize();

void Update();

void RequestScan();

void ReleaseAll();

void SetMotorSpeeds(size_t index, float low, float high);

const GamepadState &GetState(size_t index);

} // namespace ou::XInput
