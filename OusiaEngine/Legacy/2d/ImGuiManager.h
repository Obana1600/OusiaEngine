#pragma once

#ifdef USE_IMGUI
#include "externals/imgui/imgui.h"
#include "externals/imgui/imgui_stdlib.h"
#include <Windows.h>
#include <d3d12.h>
#include <wrl.h>
#endif


namespace ou {

class DirectXCommon;

class ImGuiManager {
public: // 静的メンバ関数

	static ImGuiManager *GetInstance();


public: // メンバ関数

	void Initialize(HWND hwnd);
	void Finalize();

	void Begin();
	void End();

	void Draw();


private: // メンバ変数

#ifdef USE_IMGUI
	DirectXCommon *dxCommon_ = nullptr;
	Microsoft::WRL::ComPtr<ID3D12DescriptorHeap> srvHeap_{};
#endif


private: // メンバ関数

	ImGuiManager() = default;
	~ImGuiManager() = default;
	ImGuiManager(const ImGuiManager &) = delete;
	const ImGuiManager &operator=(const ImGuiManager &) = delete;
};

} // namespace ou