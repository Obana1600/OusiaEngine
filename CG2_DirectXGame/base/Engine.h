#pragma once

#include "2d/ImGuiManager.h"
#include "2d/Sprite.h"
#include "3d/Model.h"
#include "DXCompiler.h"
#include "DirectXCommon.h"
#include "Window.h"
#include "input/InputSystem.h"

#include <WinUser.h>
#include <memory>
#include <string>
#include <cstdint>


namespace ou {

struct EngineDesc {
	std::string title;
	uint32_t width = 1280;
	uint32_t height = 720;

	bool enableDebugLayer = false;
	bool enableGpuBasedValidation = false;
	bool enableDred = false;
};


class Engine {
public: // メンバ関数

	void Initialize(const EngineDesc &desc);

	void Finalize();

	bool Update();


private: // メンバ変数

	std::unique_ptr<Window> window_;
};

} // namespace ou