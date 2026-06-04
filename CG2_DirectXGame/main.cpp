#include "base/Engine.h"
#include <Windows.h>


// Windowsアプリでのエントリーポイント（main関数）
int WINAPI WinMain(_In_ HINSTANCE, _In_opt_ HINSTANCE, _In_ LPSTR, _In_ int) {

	Engine::Initialize();

	DirectXCommon *dxCommon = DirectXCommon::GetInstance();
	ImGuiManager *imguiMgr = ImGuiManager::GetInstance();

	while (true) {
		if (Engine::Update()) {
			break;
		}

		imguiMgr->Begin();

		// ここにgameSceneのupdate

		imguiMgr->End();


		dxCommon->PreDraw();

		// ここにgameSceneのdraw

		imguiMgr->Draw();

		dxCommon->PostDraw();

	}

	Engine::Finalize();

	return 0;

}