#include "Hud.h"
#include "Asset/AssetDatabase.h"
#include "Boss.h"
#include "ComboCounter.h"
#include "Core/Time.h"
#include "MenuUi.h"
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

// ボスの HP バーの上に出す名前（形態が変わっても同じ）
constexpr const char *kBossName = "マルっこ";
constexpr float kBossBarWidth = 600.0f;
constexpr float kBossBarHeight = 18.0f;

// プレイヤーの HP は画面中央下、操作説明のすぐ上
constexpr float kPlayerBarWidth = 480.0f;
constexpr float kPlayerBarHeight = 14.0f;
constexpr float kPlayerBarBottom = 52.0f;
const Color kPlayerBarColor = {0.15f, 0.75f, 0.95f, 1.0f};
const Color kBarBackColor = {0.1f, 0.1f, 0.1f, 0.8f};

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
	// HUD の上でも引っ張りを始められるよう、クリックを受けない
	text->SetRaycastTarget(false);
	return text;
}

// anchor は画面の上端か下端の中央。position はバーの左端
Image *CreateBar(Scene &scene, Transform *canvas, const Vector2 &anchor, const Vector2 &position, const Vector2 &size, const Color &color) {
	auto *object = scene.CreateGameObject("Hud Bar");
	auto *image = object->AddComponent<Image>();
	// 左端を基準に伸び縮みさせる
	auto *rect = Place(object, canvas, anchor, position, size);
	rect->SetPivot({0.0f, anchor.y});
	image->SetColor(color);
	image->SetRaycastTarget(false);
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
	const auto font = LoadGameFont();

	comboText_ = CreateText(scene, canvas, font, {1.0f, 1.0f}, {-24.0f, -16.0f}, {360.0f, 80.0f}, 40.0f, TextAlignment::TopRight);

	// ボスの名前と開始時の大きな文字は、太い NotoSansJP Black で出す
	const auto titleFont = AssetDatabase::LoadAssetAtPath<FontAsset>("EngineAssets/Font/NotoSansJP-Black.ttf");
	auto *bossText = CreateText(scene, canvas, titleFont, {0.5f, 1.0f}, {0.0f, -8.0f}, {kBossBarWidth, 36.0f}, 26.0f, TextAlignment::Top);
	bossText->SetText(kBossName);
	const Vector2 bossBarPosition = {-kBossBarWidth * 0.5f, -46.0f};
	CreateBar(scene, canvas, {0.5f, 1.0f}, bossBarPosition, {kBossBarWidth, kBossBarHeight}, kBarBackColor);
	bossBar_ = CreateBar(scene, canvas, {0.5f, 1.0f}, bossBarPosition, {kBossBarWidth, kBossBarHeight}, {0.9f, 0.25f, 0.3f, 1.0f});

	const Vector2 playerBarPosition = {-kPlayerBarWidth * 0.5f, kPlayerBarBottom};
	CreateBar(scene, canvas, {0.5f, 0.0f}, playerBarPosition, {kPlayerBarWidth, kPlayerBarHeight}, kBarBackColor);
	playerBar_ = CreateBar(scene, canvas, {0.5f, 0.0f}, playerBarPosition, {kPlayerBarWidth, kPlayerBarHeight}, kPlayerBarColor);

	bannerText_ = CreateText(scene, canvas, titleFont, {0.5f, 0.5f}, {0.0f, 40.0f}, {1600.0f, 320.0f}, 200.0f, TextAlignment::Center);
	// 大きな文字は縁取りなしで見せる
	bannerText_->SetOutlineWidth(0.0f);
	helpText_ = CreateText(scene, canvas, font, {0.5f, 0.0f}, {0.0f, 12.0f}, {1200.0f, 36.0f}, 20.0f, TextAlignment::Bottom);
	helpText_->SetText("左ドラッグ: 引っ張る（離すと発射・右クリックで取り消し）");
}


void ou::Hud::LateUpdate() {
	if (const auto *player = Player::GetInstance()) {
		const auto ratio = std::clamp(player->GetHp() / Tuning::kPlayerMaxHp, 0.0f, 1.0f);
		dynamic_cast<RectTransform *>(playerBar_->GetTransform())->SetSizeDelta({kPlayerBarWidth * ratio, kPlayerBarHeight});
	}

	if (const auto *combo = ComboCounter::GetInstance()) {
		comboText_->SetText(combo->GetCombo() >= 2 ? std::format("{} COMBO", combo->GetCombo()) : "");
	}

	if (const auto *boss = Boss::GetInstance()) {
		const auto ratio = std::clamp(boss->GetHp() / boss->GetMaxHp(), 0.0f, 1.0f);
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
