#pragma once

#define DIRECTINPUT_VERSION 0x0800 // DirectInputのバージョン指定
#include "math/Vector2.h"
#include <Windows.h>
#include <array>
#include <dinput.h>
#include <wrl.h>


class InputManager {
public: // インナークラス

	enum MouseButton {
		LEFT,	 // 0
		RIGHT,	 // 1
		MIDDLE,	 // 2
		BUTTON3, // 3
		BUTTON4, // 4
		BUTTON5, // 5
		BUTTON6, // 6
		BUTTON7	 // 7
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
		return key_;
	}

	const DIMOUSESTATE2 &GetAllMouse() const {
		return mouse_;
	}

	const Vector2 &GetMousePosition() const {
		return mousePosition_;
	}


private: // メンバ変数

	Microsoft::WRL::ComPtr<IDirectInput8> directInput_;
	Microsoft::WRL::ComPtr<IDirectInputDevice8> devKeyboard_;
	Microsoft::WRL::ComPtr<IDirectInputDevice8> devMouse_;
	std::array<BYTE, 256> key_;
	std::array<BYTE, 256> keyPre_;
	DIMOUSESTATE2 mouse_{};
	DIMOUSESTATE2 mousePre_{};
	HWND hwnd_{};
	Vector2 mousePosition_{};


private: // メンバ関数

	InputManager() = default;
	~InputManager();
	InputManager(const InputManager &) = delete;
	const InputManager &operator=(const InputManager &) = delete;

};