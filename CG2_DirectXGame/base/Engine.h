#pragma once

#include "WinApp.h"
#include <WinUser.h>
#include "DirectXCommon.h"
#include "pso/DXCompiler.h"
#include "pso/PipelineState.h"
#include "VertexData.h"
#include "pso/RootSignature.h"
#include "2d/ImGuiManager.h"
#include "input/InputManager.h"
#include <string>


namespace Engine {

	void Initialize(const std::wstring &title = L"LE2X_99_カマタ_タロウ", bool enableDebugLayer = false);

	void Finalize();

	bool Update();

}