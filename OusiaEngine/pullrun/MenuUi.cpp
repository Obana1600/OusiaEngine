#include "MenuUi.h"
#include "GameAudio.h"
#include "Asset/AssetDatabase.h"
#include "Scene/GameObject.h"
#include "Scene/Scene.h"
#include "UI/Asset/FontAsset.h"
#include "UI/Button.h"
#include "UI/Canvas.h"
#include "UI/ColorBlock.h"
#include "UI/EventSystem/EventSystem.h"
#include "UI/EventSystem/StandaloneInputModule.h"
#include "UI/GraphicRaycaster.h"
#include "UI/Image.h"
#include "UI/RectTransform.h"
#include "UI/Text.h"

using namespace ou;


namespace {

const Vector2 kButtonSize = {360.0f, 84.0f};
constexpr float kButtonFontSize = 40.0f;

} // namespace



std::shared_ptr<FontAsset> ou::LoadGameFont() {
	return AssetDatabase::LoadAssetAtPath<FontAsset>("Assets/Font/MochiyPopOne-Regular.ttf");
}


Transform *ou::CreateMenuCanvas(Scene &scene) {
	auto *object = scene.CreateGameObject("Menu Canvas");
	object->AddComponent<Canvas>();
	object->AddComponent<GraphicRaycaster>();
	return object->GetTransform();
}


EventSystem *ou::CreateEventSystem(Scene &scene) {
	auto *object = scene.CreateGameObject("EventSystem");
	auto *eventSystem = object->AddComponent<EventSystem>();
	object->AddComponent<StandaloneInputModule>();
	return eventSystem;
}


Image *ou::CreateScreenCover(Scene &scene, Transform *parent, const Color &color) {
	auto *object = scene.CreateGameObject("Screen Cover");
	auto *image = object->AddComponent<Image>();
	image->SetColor(color);

	auto *rect = dynamic_cast<RectTransform *>(object->GetTransform());
	rect->SetParent(parent);
	rect->SetAnchorMin(Vector2::Zero);
	rect->SetAnchorMax(Vector2::One);
	rect->SetPivot({0.5f, 0.5f});
	rect->SetAnchoredPosition(Vector2::Zero);
	rect->SetSizeDelta(Vector2::Zero);
	return image;
}


Text *ou::CreateMenuText(Scene &scene, Transform *parent, const std::shared_ptr<FontAsset> &font, std::string_view text,
						 const Vector2 &position, float fontSize) {
	auto *object = scene.CreateGameObject("Menu Text");
	auto *label = object->AddComponent<Text>();
	if (font) { label->SetFont(font); }
	label->SetText(text);
	label->SetFontSize(fontSize);
	label->SetAlignment(TextAlignment::Center);
	label->SetOutlineColor(Color::Black);
	label->SetOutlineWidth(0.2f);
	// 文字はクリックを受けない（ボタンの上の文字も、下のボタンに通す）
	label->SetRaycastTarget(false);
	PlaceAtCenter(object->GetTransform(), parent, position, {1200.0f, fontSize * 2.0f});
	return label;
}


Button *ou::CreateMenuButton(Scene &scene, Transform *parent, const std::shared_ptr<FontAsset> &font, std::string_view label,
							 const Vector2 &position) {
	auto *object = scene.CreateGameObject(label);
	// Image を先に付けると、Button が Awake で targetGraphic にする
	object->AddComponent<Image>();
	PlaceAtCenter(object->GetTransform(), parent, position, kButtonSize);
	auto *button = object->AddComponent<Button>();
	// 呼び出し側の処理（シーンの切り替えなど）より先に鳴らす
	button->onClick_.AddListener([] { GameAudio::PlaySe(Se::SystemClick); });

	// Image は白のまま、ColorBlock の色で塗る。選ばれているものだけ明るい水色にする
	ColorBlock colors;
	colors.normalColor = {0.12f, 0.15f, 0.24f, 0.85f};
	colors.highlightedColor = {0.25f, 0.7f, 1.0f, 1.0f};
	colors.selectedColor = {0.25f, 0.7f, 1.0f, 1.0f};
	colors.pressedColor = {0.15f, 0.45f, 0.75f, 1.0f};
	colors.disabledColor = {0.1f, 0.1f, 0.1f, 0.5f};
	colors.fadeDuration = 0.08f;
	button->SetColors(colors);

	auto *text = CreateMenuText(scene, object->GetTransform(), font, label, Vector2::Zero, kButtonFontSize);
	dynamic_cast<RectTransform *>(text->GetTransform())->SetSizeDelta(kButtonSize);
	return button;
}


RectTransform *ou::PlaceAtCenter(Transform *object, Transform *parent, const Vector2 &position, const Vector2 &size) {
	auto *rect = dynamic_cast<RectTransform *>(object);
	rect->SetParent(parent);
	rect->SetAnchorMin({0.5f, 0.5f});
	rect->SetAnchorMax({0.5f, 0.5f});
	rect->SetPivot({0.5f, 0.5f});
	rect->SetAnchoredPosition(position);
	rect->SetSizeDelta(size);
	return rect;
}
