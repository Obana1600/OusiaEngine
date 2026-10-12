#include "GameScene.h"
#include "imgui/imgui.h"
#include "Input/InputSystem.h"
#include "Graphics/TextureManager.h"

#include <cassert>

using namespace ou;


void GameScene::Initialize() {
	const auto texHandle = TextureManager::Load("uvChecker.png");
	sprite_ = Sprite::Create(texHandle, {640.0f, 360.0f}, {1.0f, 1.0f, 1.0f, 1.0f}, {0.5f, 0.5f});
	sprite_->SetTextureRect({0.0f, 0.0f}, {256.0f, 256.0f});
	sprite_->SetSize({256.0f, 256.0f});
}


void GameScene::Update() {
	auto rotation = sprite_->GetRotation();
	rotation.x += 0.01f;
	sprite_->SetRotation(rotation);
}


void ou::GameScene::Draw(CommandList cmd) const {
	Sprite::PreDraw(cmd);
	sprite_->Draw(cmd);
}
