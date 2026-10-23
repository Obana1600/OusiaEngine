#include "TitleScene.h"
#include "Diagnostics/Log.h"
#include "Input/Input.h"
#include "Scene/GameObject.h"
#include "Scene/SceneManager.h"
#include "UI/Canvas.h"
#include "UI/RectTransform.h"
#include "UI/Text.h"

using namespace ou;


void ou::TitleScene::Initialize(Scene &scene) {
	Log::Info("TitleScene::Initialize");

	auto *canvasObject = scene.CreateGameObject("Canvas");
	canvasObject->AddComponent<Canvas>();

	auto *titleObject = scene.CreateGameObject("Title");
	auto *title = titleObject->AddComponent<Text>();
	title->SetText("OusiaEngine\nタイトルシーン\nEnter でスタート");
	title->SetFontSize(64.0f);
	title->SetAlignment(TextAlignment::Center);

	auto *rect = dynamic_cast<RectTransform *>(titleObject->GetTransform());
	rect->SetParent(canvasObject->GetTransform());
	rect->SetSizeDelta({1200.0f, 400.0f});
}


void ou::TitleScene::Update() {
	if (Input::GetKeyDown(KeyCode::Return)) {
		SceneManager::LoadScene("Game");
	}
}


void ou::TitleScene::Finalize() {
	Log::Info("TitleScene::Finalize");
}
