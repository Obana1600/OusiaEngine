#include "TitleMenu.h"
#include "Asset/AssetDatabase.h"
#include "Core/Application.h"
#include "Core/Time.h"
#include "MenuUi.h"
#include "Math/Mathf.h"
#include "Render/Asset/Texture.h"
#include "RicochetScene.h"
#include "Scene/GameObject.h"
#include "Scene/Scene.h"
#include "Scene/SceneManager.h"
#include "UI/Asset/Sprite.h"
#include "UI/Button.h"
#include "UI/EventSystem/EventSystem.h"
#include "UI/Image.h"
#include "UI/RectTransform.h"

using namespace ou;


namespace {

const Vector2 kLogoPosition = {0.0f, 150.0f};
const Vector2 kPlayPosition = {0.0f, -120.0f};
const Vector2 kQuitPosition = {0.0f, -220.0f};

// ロゴをゆっくり上下させる幅（px）と速さ
constexpr float kLogoBobHeight = 8.0f;
constexpr float kLogoBobSpeed = 1.5f;

} // namespace



void ou::TitleMenu::Start() {
	auto &scene = *GetGameObject()->GetScene();
	auto *eventSystem = CreateEventSystem(scene);
	auto *canvas = CreateMenuCanvas(scene);
	const auto font = LoadMenuFont();

	auto *logoObject = scene.CreateGameObject("Logo");
	auto *logo = logoObject->AddComponent<Image>();
	logo->SetRaycastTarget(false);
	if (auto texture = AssetDatabase::LoadAssetAtPath<Texture>("Assets/title.png")) { logo->SetSprite(Sprite::Create(texture)); }
	logo_ = PlaceAtCenter(logoObject->GetTransform(), canvas, kLogoPosition, Vector2::Zero);
	logo->SetNativeSize();
	logoPosition_ = kLogoPosition;

	auto *play = CreateMenuButton(scene, canvas, font, "プレイ", kPlayPosition);
	play->onClick_.AddListener([] { SceneManager::LoadScene(kRicochetSceneName); });
	auto *quit = CreateMenuButton(scene, canvas, font, "終了", kQuitPosition);
	quit->onClick_.AddListener([] { Application::Quit(); });

	eventSystem->SetFirstSelectedGameObject(play->GetGameObject());
}


void ou::TitleMenu::Update() {
	time_ += Time::GetUnscaledDeltaTime();
	logo_->SetAnchoredPosition(logoPosition_ + Vector2{0.0f, Mathf::Sin(time_ * kLogoBobSpeed) * kLogoBobHeight});
}
