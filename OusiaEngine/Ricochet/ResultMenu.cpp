#include "ResultMenu.h"
#include "Core/Time.h"
#include "MenuUi.h"
#include "RicochetScene.h"
#include "Scene/GameObject.h"
#include "Scene/Scene.h"
#include "Scene/SceneManager.h"
#include "TitleScene.h"
#include "UI/Button.h"
#include "UI/EventSystem/EventSystem.h"
#include "UI/Image.h"
#include "UI/RectTransform.h"
#include "UI/Text.h"

#include <algorithm>

using namespace ou;


namespace {

const Vector2 kTitlePosition = {0.0f, 150.0f};
const Vector2 kSubtitlePosition = {0.0f, 60.0f};
const Vector2 kRetryPosition = {0.0f, -120.0f};
const Vector2 kToTitlePosition = {0.0f, -220.0f};

constexpr float kTitleFontSize = 96.0f;
constexpr float kSubtitleFontSize = 36.0f;

// 暗くなりきったときの幕の濃さと、そこまでの時間
constexpr float kCoverAlpha = 0.65f;
constexpr float kFadeTime = 0.5f;

} // namespace



void ou::ResultMenu::Awake() {
	sInstance_ = this;
}


void ou::ResultMenu::Start() {
	auto &scene = *GetGameObject()->GetScene();
	eventSystem_ = CreateEventSystem(scene);
	auto *canvas = CreateMenuCanvas(scene);
	const auto font = LoadMenuFont();

	// 幕を親にして、幕ごと出し入れする
	cover_ = CreateScreenCover(scene, canvas, {0.0f, 0.0f, 0.0f, 0.0f});
	auto *root = cover_->GetTransform();
	titleText_ = CreateMenuText(scene, root, font, "", kTitlePosition, kTitleFontSize);
	subtitleText_ = CreateMenuText(scene, root, font, "", kSubtitlePosition, kSubtitleFontSize);

	retryButton_ = CreateMenuButton(scene, root, font, "リトライ", kRetryPosition);
	retryButton_->onClick_.AddListener([] { SceneManager::LoadScene(kRicochetSceneName); });
	titleButton_ = CreateMenuButton(scene, root, font, "タイトルへ", kToTitlePosition);
	titleButton_->onClick_.AddListener([] { SceneManager::LoadScene(kTitleSceneName); });

	SetMenuVisible(false);
}


void ou::ResultMenu::Update() {
	if (!isShown_) { return; }

	// クリアやゲームオーバーの瞬間はスローやヒットストップがかかっていることがあるので、実時間で進める
	const auto dt = Time::GetUnscaledDeltaTime();
	if (!cover_->GetGameObject()->IsActiveSelf()) {
		delay_ -= dt;
		if (delay_ > 0.0f) { return; }
		SetMenuVisible(true);
		eventSystem_->SetSelectedGameObject(retryButton_->GetGameObject());
	}

	time_ += dt;
	const auto ratio = std::clamp(time_ / kFadeTime, 0.0f, 1.0f);
	cover_->SetColor({0.0f, 0.0f, 0.0f, kCoverAlpha * ratio});

	// 見出しは上から少し降りてくる
	const auto eased = 1.0f - (1.0f - ratio) * (1.0f - ratio);
	dynamic_cast<RectTransform *>(titleText_->GetTransform())->SetAnchoredPosition(kTitlePosition + Vector2{0.0f, 40.0f * (1.0f - eased)});
	titleText_->SetColor({1.0f, 1.0f, 1.0f, ratio});
	subtitleText_->SetColor({1.0f, 1.0f, 1.0f, ratio});
}


void ou::ResultMenu::OnDestroy() {
	if (sInstance_ == this) { sInstance_ = nullptr; }
}


void ou::ResultMenu::Show(std::string_view title, std::string_view subtitle, float delay) {
	if (isShown_) { return; }
	isShown_ = true;
	delay_ = delay;
	time_ = 0.0f;
	titleText_->SetText(title);
	subtitleText_->SetText(subtitle);
}


void ou::ResultMenu::SetMenuVisible(bool visible) {
	cover_->GetGameObject()->SetActive(visible);
}
