#pragma once

#define DIRECTINPUT_VERSION 0x0800 // DirectInputのバージョン指定
#include <dinput.h>
#include <wrl.h>
#include <array>
#include <Windows.h>
#include "math/Vector2.h"


class InputManager {
public: // インナークラス

	enum MouseButton {
		LEFT, // 0
		RIGHT, // 1
		MIDDLE, // 2
		BUTTON3, // 3
		BUTTON4, // 4
		BUTTON5, // 5
		BUTTON6, // 6
		BUTTON7 // 7
	};

	struct MouseMove {
		int32_t x;
		int32_t y;
		int32_t z;
	};


public: // 静的メンバ関数

	static InputManager *GetInstance();


public: // メンバ関数

	void Initialize();

	void Update();

	bool PushKey(BYTE keyNumber) const;

	bool TriggerKey(BYTE keyNumber) const;

	bool PressMouse(MouseButton buttonNumber) const;

	bool TriggerMouse(MouseButton buttonNumber) const;

	MouseMove GetMouseMove() const;

	int32_t GetWheel() const;

	const std::array<BYTE, 256> &GetAllKey() const {
		return m_key;
	}

	const DIMOUSESTATE2 &GetAllMouse() const {
		return m_mouse;
	}

	const Vector2 &GetMousePosition() const {
		return m_mousePosition;
	}


private: // メンバ変数

	Microsoft::WRL::ComPtr<IDirectInput8> m_directInput{};
	Microsoft::WRL::ComPtr<IDirectInputDevice8> m_devKeyboard{};
	Microsoft::WRL::ComPtr<IDirectInputDevice8> m_devMouse{};
	std::array<BYTE, 256> m_key{};
	std::array<BYTE, 256> m_keyPre{};
	DIMOUSESTATE2 m_mouse{};
	DIMOUSESTATE2 m_mousePre{};
	HWND m_hwnd{};
	Vector2 m_mousePosition{};


private: // メンバ関数

	InputManager() = default;
	~InputManager();
	InputManager(const InputManager &) = delete;
	const InputManager &operator=(const InputManager &) = delete;

}; // InputManager