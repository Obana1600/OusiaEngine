#include "MyGame.h"
#include "GameScene.h"
#include "Scene/SceneManager.h"

using namespace ou;


ou::MyGame::MyGame()
	: Framework({.title = "OusiaEngine"}) {}


void ou::MyGame::Initialize() {
	Framework::Initialize();

	SceneManager::AddScene("Game", BuildGameScene);
	SceneManager::LoadScene("Game");
}
