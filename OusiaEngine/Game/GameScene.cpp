#include "GameScene.h"
#include "Diagnostics/Log.h"
#include "Input/Input.h"
#include "Scene/GameObject.h"
#include "Scene/SceneManager.h"
#include "UI/Canvas.h"
#include "UI/RectTransform.h"
#include "UI/Text.h"

using namespace ou;



void ou::GameScene::Initialize(Scene &scene) {
	Log::Info("GameScene::Initialize");

	auto *canvasObject = scene.CreateGameObject("Canvas");
	canvasObject->AddComponent<Canvas>();

	auto *titleObject = scene.CreateGameObject("Title");
	auto *title = titleObject->AddComponent<Text>();
	title->SetText("OusiaEngine\nゲームシーン\nEscape でタイトルへ");
	title->SetFontSize(64.0f);
	title->SetAlignment(TextAlignment::Center);

	auto *rect = dynamic_cast<RectTransform *>(titleObject->GetTransform());
	rect->SetParent(canvasObject->GetTransform());
	rect->SetSizeDelta({1200.0f, 400.0f});
}


void ou::GameScene::Update() {
	if (Input::GetKeyDown(KeyCode::Escape)) {
		SceneManager::LoadScene("Title");
	}
}


void ou::GameScene::Finalize() {
	Log::Info("GameScene::Finalize");
}
