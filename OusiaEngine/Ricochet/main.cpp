#include "Engine.h"
#include "RicochetScene.h"
#include "Scene/SceneManager.h"

#include <Windows.h>


// Windowsアプリでのエントリーポイント（main関数）
int WINAPI WinMain(_In_ HINSTANCE, _In_opt_ HINSTANCE, _In_ LPSTR, _In_ int) {
	using namespace ou;

	Engine engine;
	engine.Initialize({.title = "Ricochet"});

	SceneManager::AddScene(kRicochetSceneName, BuildRicochetScene);
	SceneManager::LoadScene(kRicochetSceneName);

	while (!engine.Update()) {
		SceneManager::Update();

		auto cmd = engine.BeginDraw();
		if (!cmd.IsValid()) { continue; }

		SceneManager::Render(cmd);

		engine.BeginOverlay(cmd);
		SceneManager::RenderOverlay(cmd);

		engine.EndDraw(cmd);
	}

	engine.Finalize();

	return 0;
}
