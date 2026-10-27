#include "Hud.h"
#include "Asset/AssetDatabase.h"
#include "Boss.h"
#include "ComboCounter.h"
#include "Core/Time.h"
#include "Player.h"
#include "Scene/GameObject.h"
#include "Scene/Scene.h"
#include "Tuning.h"
#include "UI/Canvas.h"
#include "UI/Asset/FontAsset.h"
#include "UI/Image.h"
#include "UI/RectTransform.h"
#include "UI/Text.h"

#include <algorithm>
#include <format>

using namespace ou;


namespace {

constexpr float kBossBarWidth = 600.0f;
constexpr float kBossBarHeight = 18.0f;

RectTransform *Place(GameObject *object, Transform *canvas, const Vector2 &anchor, const Vector2 &position, const Vector2 &size) {
	auto *rect = dynamic_cast<RectTransform *>(object->GetTransform());
	rect->SetParent(canvas);
	rect->SetAnchorMin(anchor);
	rect->SetAnchorMax(anchor);
	rect->SetPivot(anchor);
	rect->SetAnchoredPosition(position);
	rect->SetSizeDelta(size);
	return rect;
}

Text *CreateText(Scene &scene, Transform *canvas, const std::shared_ptr<FontAsset> &font, const Vector2 &anchor,
				 const Vector2 &position, const Vector2 &size, float fontSize, TextAlignment alignment) {
	auto *object = scene.CreateGameObject("Hud Text");
	auto *text = object->AddComponent<Text>();
	Place(object, canvas, anchor, position, size);
	if (font) { text->SetFont(font); }
	text->SetFontSize(fontSize);
	text->SetAlignment(alignment);
	text->SetOutlineColor(Color::Black);
	text->SetOutlineWidth(0.2f);
	return text;
}

Image *CreateBar(Scene &scene, Transform *canvas, const Vector2 &position, const Vector2 &size, const Color &color) {
	auto *object = scene.CreateGameObject("Hud Bar");
	auto *image = object->AddComponent<Image>();
	// 左端を基準に伸び縮みさせる
	auto *rect = Place(object, canvas, {0.5f, 1.0f}, position, size);
	rect->SetPivot({0.0f, 1.0f});
	image->SetColor(color);
	return image;
}

} // namespace



void ou::Hud::Awake() {
	sInstance_ = this;
}


void ou::Hud::Start() {
	auto &scene = *GetGameObject()->GetScene();
	auto *canvasObject = scene.CreateGameObject("Canvas");
	canvasObject->AddComponent<Canvas>();
	auto *canvas = canvasObject->GetTransform();
	const auto font = AssetDatabase::LoadAssetAtPath<FontAsset>("EngineAssets/Font/NotoSansJP-Bold.ttf");

	playerText_ = CreateText(scene, canvas, font, {0.0f, 1.0f}, {24.0f, -16.0f}, {420.0f, 40.0f}, 28.0f, TextAlignment::TopLeft);
	stockText_ = CreateText(scene, canvas, font, {0.0f, 1.0f}, {24.0f, -56.0f}, {420.0f, 40.0f}, 28.0f, TextAlignment::TopLeft);
	comboText_ = CreateText(scene, canvas, font, {1.0f, 1.0f}, {-24.0f, -16.0f}, {360.0f, 80.0f}, 40.0f, TextAlignment::TopRight);

	bossText_ = CreateText(scene, canvas, font, {0.5f, 1.0f}, {0.0f, -12.0f}, {kBossBarWidth, 32.0f}, 22.0f, TextAlignment::Top);
	const Vector2 barPosition = {-kBossBarWidth * 0.5f, -46.0f};
	bossBarBack_ = CreateBar(scene, canvas, barPosition, {kBossBarWidth, kBossBarHeight}, {0.1f, 0.1f, 0.1f, 0.8f});
	bossBar_ = CreateBar(scene, canvas, barPosition, {kBossBarWidth, kBossBarHeight}, {0.9f, 0.25f, 0.3f, 1.0f});

	bannerText_ = CreateText(scene, canvas, font, {0.5f, 0.5f}, {0.0f, 60.0f}, {1000.0f, 200.0f}, 64.0f, TextAlignment::Center);
	helpText_ = CreateText(scene, canvas, font, {0.5f, 0.0f}, {0.0f, 12.0f}, {1200.0f, 36.0f}, 20.0f, TextAlignment::Bottom);
	helpText_->SetText("WASD / 左スティック: 引っ張って溜める　SPACE / A: 発射（離すと取り消し）　R: リトライ");
}


void ou::Hud::LateUpdate() {
	if (const auto *player = Player::GetInstance()) {
		playerText_->SetText(std::format("HP {:.0f} / {:.0f}", player->GetHp(), Tuning::kPlayerMaxHp));
		std::string stock;
		for (int i = 0; i < Tuning::kMaxStock; i++) { stock += i < player->GetStock() ? "●" : "○"; }
		stockText_->SetText("STOCK " + stock);
	}

	if (const auto *combo = ComboCounter::GetInstance()) {
		comboText_->SetText(combo->GetCombo() >= 2 ? std::format("{} COMBO", combo->GetCombo()) : "");
	}

	if (const auto *boss = Boss::GetInstance()) {
		const auto ratio = std::clamp(boss->GetHp() / boss->GetMaxHp(), 0.0f, 1.0f);
		bossText_->SetText(std::format("BOSS 第{}形態", boss->GetPhase() == 1 ? "一" : "二"));
		dynamic_cast<RectTransform *>(bossBar_->GetTransform())->SetSizeDelta({kBossBarWidth * ratio, kBossBarHeight});
	}

	if (bannerTimer_ > 0.0f) {
		bannerTimer_ -= Time::GetUnscaledDeltaTime();
		if (bannerTimer_ <= 0.0f) { bannerText_->SetText(""); }
	}
}


void ou::Hud::OnDestroy() {
	if (sInstance_ == this) { sInstance_ = nullptr; }
}


void ou::Hud::ShowBanner(const std::string &message, float duration) {
	if (!bannerText_) { return; }
	bannerText_->SetText(message);
	bannerTimer_ = duration;
}
