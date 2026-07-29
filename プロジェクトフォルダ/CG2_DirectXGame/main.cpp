#include "base/Engine.h"
#include "GameScene.h"
#include <Windows.h>


// Windowsアプリでのエントリーポイント（main関数）
int WINAPI WinMain(_In_ HINSTANCE, _In_opt_ HINSTANCE, _In_ LPSTR, _In_ int) {

	Engine::Initialize(L"LE2B_05_オバナ_サトル_CG2");

	DirectXCommon *dxCommon = DirectXCommon::GetInstance();
	ImGuiManager *imguiMgr = ImGuiManager::GetInstance();

	// GameSceneの初期化
	GameScene gameScene{};
	gameScene.Initialize();

	dxCommon->ExecuteTransferCommands();

	while (true) {
		if (Engine::Update()) {
			break;
		}

		imguiMgr->Begin();

		gameScene.Update();

		imguiMgr->End();


		dxCommon->PreDraw();

		gameScene.Draw();

		imguiMgr->Draw();

		dxCommon->PostDraw();

	}

	Engine::Finalize();

	return 0;
}