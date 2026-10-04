#pragma once

#include "3d/Camera.h"
#include "3d/DebugCamera.h"


class GameScene {
public: // メンバ関数

	void Initialize();

	void Update();

	void Draw() const;


private: // メンバ変数

	// camera
	DebugCamera camera_{};


private: // メンバ関数

	void OnImGui();
};