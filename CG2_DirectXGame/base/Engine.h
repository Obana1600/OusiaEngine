#pragma once

#include "2d/ImGuiManager.h"
#include "2d/Sprite.h"
#include "3d/Model.h"
#include "DXCompiler.h"
#include "DirectXCommon.h"
#include "WinApp.h"
#include "input/InputManager.h"
#include <WinUser.h>
#include <string>


namespace Engine {

	void Initialize(const std::wstring &title = L"DirectXGame", bool enableDebugLayer = false);

	void Finalize();

	bool Update();

} // namespace Engine