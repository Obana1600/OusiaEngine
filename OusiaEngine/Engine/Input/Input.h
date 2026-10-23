#pragma once

#include "Input/GamepadButton.h"
#include "Input/KeyCode.h"
#include "Math/Vector2.h"

// 前方宣言
namespace ou {
class Window;
}


namespace ou::Input {

// ----- 初期化 -----

bool Initialize(Window *window);
void Finalize();

void Update();


// ----- キーボード -----

bool GetKey(KeyCode key);
bool GetKeyDown(KeyCode key);
bool GetKeyUp(KeyCode key);


// ----- マウス -----

// 0: 左, 1: 右, 2: 中, 3/4: サイド
bool GetMouseButton(int button);
bool GetMouseButtonDown(int button);
bool GetMouseButtonUp(int button);

Vector2 GetMousePosition();
Vector2 GetMouseDelta();
Vector2 GetMouseScrollDelta();


// ----- ゲームパッド -----

// player: 0~3
bool IsGamepadConnected(int player = 0);

bool GetGamepadButton(GamepadButton button, int player = 0);
bool GetGamepadButtonDown(GamepadButton button, int player = 0);
bool GetGamepadButtonUp(GamepadButton button, int player = 0);

Vector2 GetGamepadLeftStick(int player = 0);
Vector2 GetGamepadRightStick(int player = 0);

float GetGamepadLeftTrigger(int player = 0);
float GetGamepadRightTrigger(int player = 0);

void SetGamepadMotorSpeeds(float lowFrequency, float highFrequency, int player = 0);

} // namespace ou::Input
