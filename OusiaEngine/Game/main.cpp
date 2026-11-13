#include "Engine.h"
#include "GameScene.h"
#include "Scene/SceneManager.h"

#include <Windows.h>


// Windowsアプリでのエントリーポイント（main関数）
int WINAPI WinMain(_In_ HINSTANCE, _In_opt_ HINSTANCE, _In_ LPSTR, _In_ int) {
	using namespace ou;

	Engine engine;
	engine.Initialize({.title = "OusiaEngine", .productName = "OusiaEngine", .projectDir = PROJECT_DIR});

	SceneManager::AddScene("Game", BuildGameScene);
	SceneManager::LoadScene("Game");

	engine.Run();

	engine.Finalize();

	return 0;
}
