#pragma once

#include "Input/GamepadButton.h"
#include "Input/KeyCode.h"
#include "Math/Vector2.h"


namespace ou {

// 前方宣言
class Window;

class Input final {
	friend class Engine;

public: // 静的メンバ関数

	Input() = delete;

	// ----- キーボード（KeyCode::JoystickButton0 などのパッドのボタンも取れる） -----

	static bool GetKey(KeyCode key);
	static bool GetKeyDown(KeyCode key);
	static bool GetKeyUp(KeyCode key);


	// ----- マウス -----

	// 0: 左, 1: 右, 2: 中, 3/4: サイド
	static bool GetMouseButton(int button);
	static bool GetMouseButtonDown(int button);
	static bool GetMouseButtonUp(int button);

	static Vector2 GetMousePosition();
	static Vector2 GetMouseDelta();
	static Vector2 GetMouseScrollDelta();


	// ----- ゲームパッド -----

	// player: 0 ~ 3
	static bool IsGamepadConnected(int player = 0);

	static bool GetGamepadButton(GamepadButton button, int player = 0);
	static bool GetGamepadButtonDown(GamepadButton button, int player = 0);
	static bool GetGamepadButtonUp(GamepadButton button, int player = 0);

	static Vector2 GetGamepadLeftStick(int player = 0);
	static Vector2 GetGamepadRightStick(int player = 0);

	static float GetGamepadLeftTrigger(int player = 0);
	static float GetGamepadRightTrigger(int player = 0);

	static void SetGamepadMotorSpeeds(float lowFrequency, float highFrequency, int player = 0);


private: // 静的メンバ関数

	static bool Initialize(Window *window);
	static void Finalize();

	static void Update();

#ifdef WITH_EDITOR
	static void SetGameView(const Vector2 &origin, bool hasFocus, bool isHovered);
#endif
};

} // namespace ou