#include "GameScene.h"

#include "input/InputManager.h"
#include "externals/imgui/imgui.h"
#include <cassert>


void GameScene::Initialize() {
	camera_.Initialize();
}


void GameScene::Update() {
	OnImGui();
}


void GameScene::Draw() const {
}


void GameScene::OnImGui() {
	ImGui::Begin("Inputの確認");

	if (InputManager::GetInstance()->PushKey(DIK_SPACE)) {
		ImGui::Text("Hit");
	}
	
	ImGui::End();
}