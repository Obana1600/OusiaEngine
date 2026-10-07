#include "GameScene.h"
#include "base/Engine.h"
#include <Windows.h>
#include <Graphics/GraphicsDevice_DX12.h>


// Windowsアプリでのエントリーポイント（main関数）
int WINAPI WinMain(_In_ HINSTANCE, _In_opt_ HINSTANCE, _In_ LPSTR, _In_ int) {

	using namespace ou;

	Engine *engine = new Engine();
	engine->Initialize({.title = "LE2A_オバナ_サトル"});
	GraphicsDevice_DX12 testDevice(ValidationMode::Enabled);
	for (int i = 0; i < 10; i++) {
		testDevice.BeginCommandList();
		testDevice.BeginCommandList(); // 2 本借りても大丈夫か
		testDevice.SubmitCommandLists();
	}
	DirectXCommon *dxCommon = DirectXCommon::GetInstance();
	ImGuiManager *imguiMgr = ImGuiManager::GetInstance();

	// GameSceneの初期化
	GameScene gameScene{};
	gameScene.Initialize();

	dxCommon->ExecuteTransferCommands();

	while (true) {
		if (engine->Update()) {
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

	engine->Finalize();

	return 0;
}