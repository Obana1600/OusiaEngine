#include "Engine.h"
#include "GameScene.h"

#include <Windows.h>


// Windowsアプリでのエントリーポイント（main関数）
int WINAPI WinMain(_In_ HINSTANCE, _In_opt_ HINSTANCE, _In_ LPSTR, _In_ int) {
	using namespace ou;

	Engine engine;
	engine.Initialize({.title = "OusiaEngine"});
	
	{
		GameScene gameScene;
		gameScene.Initialize();

		while (!engine.Update()) {
			gameScene.Update();

			// 最小化中は無効なハンドルが返るので、描画を飛ばす
			Graphics::CommandList cmd = engine.BeginDraw();
			if (!cmd.IsValid()) { continue; }

			gameScene.Draw(cmd);

			engine.EndDraw(cmd);
		}
	} // GameScene が持つリソースは、デバイス（Engine）より先に手放す

	engine.Finalize();

	return 0;
}
