#include "GameScene.h"

#include "externals/imgui/imgui.h"
#include "input/InputSystem.h"
#include <cassert>

using namespace ou;


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

	if (InputSystem::GetInstance()->PushKey(DIK_SPACE)) {
		ImGui::Text("Hit");
	}

	ImGui::End();
}