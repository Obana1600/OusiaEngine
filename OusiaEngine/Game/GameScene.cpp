#include "GameScene.h"
#include "Asset/AssetDatabase.h"
#include "Asset/Sprite.h"
#include "Asset/Texture.h"
#include "Math/Mathf.h"
#include "Scene/Camera.h"
#include "Scene/DirectionalLight.h"
#include "Scene/RectTransform.h"
#include "Scene/Transform.h"
#include "UI/Canvas.h"
#include "UI/Image.h"

#include <imgui/imgui.h>

using namespace ou;


void GameScene::Initialize() {
	scene_ = std::make_unique<Scene>();

	auto *cameraObject = scene_->CreateGameObject("Main Camera");
	cameraObject->AddComponent<Camera>();
	cameraObject->GetTransform()->SetLocalPosition({0.0f, 1.0f, -5.0f});
	cameraObject->GetTransform()->SetLocalEulerAngles({15.0f * Mathf::Deg2Rad, 0.0f, 0.0f});

	auto *lightObject = scene_->CreateGameObject("Directional Light");
	lightObject->AddComponent<DirectionalLight>();
	lightObject->GetTransform()->SetLocalEulerAngles({50.0f * Mathf::Deg2Rad, -30.0f * Mathf::Deg2Rad, 0.0f});

	// UI
	auto *canvasObject = scene_->CreateGameObject("Canvas");
	canvasObject->AddComponent<Canvas>();
	auto *canvasTransform = canvasObject->GetTransform();

	// 左上に 20 ピクセル離して uvChecker
	{
		auto *imageObject = scene_->CreateGameObject("UV Checker");
		uvChecker_ = imageObject->AddComponent<Image>();
		uvChecker_->SetSprite(Sprite::Create(AssetDatabase::LoadAssetAtPath<Texture>("Assets/uvChecker.png")));

		auto *rectTransform = imageObject->GetComponent<RectTransform>();
		rectTransform->SetParent(canvasTransform);
		rectTransform->SetAnchorMin({0.0f, 1.0f});
		rectTransform->SetAnchorMax({0.0f, 1.0f});
		rectTransform->SetPivot({0.0f, 1.0f});
		rectTransform->SetAnchoredPosition({20.0f, -20.0f});
		rectTransform->SetSizeDelta({256.0f, 256.0f});
	}
}


void GameScene::Update() {
	ShowUvCheckerWindow();

	scene_->Update();
}


void ou::GameScene::Draw(Graphics::CommandList cmd) const {
	scene_->Render(cmd);
}


void ou::GameScene::DrawOverlay(Graphics::CommandList cmd) const {
	scene_->RenderOverlay(cmd);
}


void ou::GameScene::ShowUvCheckerWindow() {
#ifdef USE_IMGUI
	auto *rectTransform = uvChecker_->GetGameObject()->GetComponent<RectTransform>();

	// 初回だけ右上に置く（2 回目からは imgui.ini に保存された位置と大きさ）
	ImGui::SetNextWindowPos({880.0f, 20.0f}, ImGuiCond_FirstUseEver);
	ImGui::SetNextWindowSize({380.0f, 360.0f}, ImGuiCond_FirstUseEver);
	ImGui::Begin("UV Checker");

	if (ImGui::CollapsingHeader("RectTransform", ImGuiTreeNodeFlags_DefaultOpen)) {
		auto anchoredPosition = rectTransform->GetAnchoredPosition();
		if (ImGui::DragFloat2("位置", &anchoredPosition.x)) {
			rectTransform->SetAnchoredPosition(anchoredPosition);
		}

		auto sizeDelta = rectTransform->GetSizeDelta();
		if (ImGui::DragFloat2("大きさ", &sizeDelta.x, 1.0f, 0.0f, 4096.0f)) {
			rectTransform->SetSizeDelta(sizeDelta);
		}

		auto pivot = rectTransform->GetPivot();
		if (ImGui::SliderFloat2("ピボット", &pivot.x, 0.0f, 1.0f)) {
			rectTransform->SetPivot(pivot);
		}

		auto anchorMin = rectTransform->GetAnchorMin();
		if (ImGui::SliderFloat2("アンカー（最小）", &anchorMin.x, 0.0f, 1.0f)) {
			rectTransform->SetAnchorMin(anchorMin);
		}

		auto anchorMax = rectTransform->GetAnchorMax();
		if (ImGui::SliderFloat2("アンカー（最大）", &anchorMax.x, 0.0f, 1.0f)) {
			rectTransform->SetAnchorMax(anchorMax);
		}

		// UI の回転は Z 軸まわりだけ。pivot が回転の中心になる
		auto rotation = rectTransform->GetLocalEulerAngles().z * Mathf::Rad2Deg;
		if (ImGui::SliderFloat("回転", &rotation, -180.0f, 180.0f, "%.1f 度")) {
			rectTransform->SetLocalEulerAngles({0.0f, 0.0f, rotation * Mathf::Deg2Rad});
		}

		const auto &localScale = rectTransform->GetLocalScale();
		Vector2 scale = {localScale.x, localScale.y};
		if (ImGui::DragFloat2("拡大縮小", &scale.x, 0.01f)) {
			rectTransform->SetLocalScale({scale.x, scale.y, localScale.z});
		}
	}

	if (ImGui::CollapsingHeader("Image", ImGuiTreeNodeFlags_DefaultOpen)) {
		// 色は sRGB のまま渡す（Image が描くときに Linear() する）
		auto color = uvChecker_->GetColor();
		if (ImGui::ColorEdit4("色", &color.r)) {
			uvChecker_->SetColor(color);
		}

		if (ImGui::Button("元の大きさにする")) {
			uvChecker_->SetNativeSize();
		}
	}

	ImGui::End();
#endif
}
