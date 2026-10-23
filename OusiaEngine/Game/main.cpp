#include "Engine.h"
#include "SceneFactory.h"
#include "Scene/SceneManager.h"

#include <Windows.h>


// Windowsアプリでのエントリーポイント（main関数）
int WINAPI WinMain(_In_ HINSTANCE, _In_opt_ HINSTANCE, _In_ LPSTR, _In_ int) {
	using namespace ou;

	Engine engine;
	engine.Initialize({.title = "OusiaEngine"});

	SceneManager::SetSceneFactory(std::make_unique<SceneFactory>());
	SceneManager::LoadScene("Title");

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
