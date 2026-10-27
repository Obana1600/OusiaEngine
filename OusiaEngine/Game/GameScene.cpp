#include "GameScene.h"
#include "Asset/AssetDatabase.h"
#include "UI/Button.h"
#include "Core/Application.h"
#include "Diagnostics/Log.h"
#include "Input/Input.h"
#include "Render/Camera.h"
#include "Scene/GameObject.h"
#include "Scene/Transform.h"
#include "UI/Asset/FontAsset.h"
#include "UI/Canvas.h"
#include "UI/CanvasRenderer.h"
#include "UI/EventSystem/EventSystem.h"
#include "UI/EventSystem/StandaloneInputModule.h"
#include "UI/GraphicRaycaster.h"
#include "UI/Image.h"
#include "UI/RectTransform.h"
#include "UI/Text.h"

#include <string>

using namespace ou;


namespace {

Button *CreateButton(Scene &scene, Transform *canvas, const std::shared_ptr<FontAsset> &font, const std::string &label, float y) {
	auto *object = scene.CreateGameObject(label);
	object->AddComponent<Image>()->SetColor({0.25f, 0.45f, 0.85f, 1.0f});
	auto *rect = dynamic_cast<RectTransform *>(object->GetTransform());
	rect->SetParent(canvas);
	rect->SetAnchoredPosition({0.0f, y});
	rect->SetSizeDelta({320.0f, 64.0f});

	// 文字も raycastTarget のまま。当たったら親の Button に届くかを見る
	auto *textObject = scene.CreateGameObject(label + " Text");
	auto *text = textObject->AddComponent<Text>();
	auto *textRect = dynamic_cast<RectTransform *>(textObject->GetTransform());
	textRect->SetParent(rect);
	textRect->SetSizeDelta({320.0f, 64.0f});
	if (font) { text->SetFont(font); }
	text->SetFontSize(28.0f);
	text->SetAlignment(TextAlignment::Center);
	text->SetText(label);

	auto *button = object->AddComponent<Button>();
	ColorBlock colors{};
	colors.highlightedColor = {1.0f, 1.0f, 0.6f, 1.0f};
	colors.selectedColor = {1.0f, 0.8f, 0.3f, 1.0f};
	colors.pressedColor = {0.4f, 0.4f, 0.4f, 1.0f};
	button->SetColors(colors);
	return button;
}

} // namespace


void ou::BuildGameScene(Scene &scene) {
	scene.CreateGameObject("UI Input Test")->AddComponent<UiInputTest>();
}


void ou::UiInputTest::Start() {
	auto &scene = *GetGameObject()->GetScene();

	scene.CreateGameObject("Main Camera")->AddComponent<Camera>();

	auto *eventSystemObject = scene.CreateGameObject("EventSystem");
	auto *eventSystem = eventSystemObject->AddComponent<EventSystem>();
	eventSystemObject->AddComponent<StandaloneInputModule>();

	auto *canvasObject = scene.CreateGameObject("Canvas");
	canvasObject->AddComponent<Canvas>();
	canvasObject->AddComponent<GraphicRaycaster>();
	auto *canvas = canvasObject->GetTransform();

	// 当たらない背景
	auto *panelObject = scene.CreateGameObject("Panel");
	auto *panel = panelObject->AddComponent<Image>();
	panel->SetColor({0.0f, 0.0f, 0.0f, 0.5f});
	panel->SetRaycastTarget(false);
	auto *panelRect = dynamic_cast<RectTransform *>(panelObject->GetTransform());
	panelRect->SetParent(canvas);
	panelRect->SetSizeDelta({400.0f, 400.0f});

	const auto font = AssetDatabase::LoadAssetAtPath<FontAsset>("EngineAssets/Font/NotoSansJP-Bold.ttf");
	auto *play = CreateButton(scene, canvas, font, "Play", 120.0f);
	auto *disabled = CreateButton(scene, canvas, font, "Disabled", 40.0f);
	auto *retry = CreateButton(scene, canvas, font, "Retry", -40.0f);
	auto *quit = CreateButton(scene, canvas, font, "Quit", -120.0f);
	disabled->SetInteractable(false);

	play->onClick_.AddListener([] { Log::Info("TEST onClick Play"); });
	disabled->onClick_.AddListener([] { Log::Info("TEST onClick Disabled (呼ばれてはいけない)"); });
	retry->onClick_.AddListener([] { Log::Info("TEST onClick Retry"); });
	quit->onClick_.AddListener([] {
		Log::Info("TEST onClick Quit -> Application::Quit");
		Application::Quit();
	});

	for (auto *button : {play, disabled, retry, quit}) {
		watched_.push_back({button->GetTargetGraphic()});
	}

	eventSystem->SetFirstSelectedGameObject(play->GetGameObject());
}


void ou::UiInputTest::Update() {
	frame_++;

	// 色が変わるあいだも、頂点は作り直されないはず
	int geometryCount = 0;
	for (auto &watched : watched_) {
		const void *vertices = watched.graphic->GetCanvasRenderer()->GetVertices().data();
		if (vertices != watched.vertices) { geometryCount++; }
		watched.vertices = vertices;
	}
	if (geometryCount > 0) {
		Log::Info("TEST frame {} rebuilt geometry={}", frame_, geometryCount);
	}

	auto *eventSystem = EventSystem::GetCurrent();
	auto *selected = eventSystem->GetCurrentSelectedGameObject();
	if (selected != lastSelected_) {
		Log::Info("TEST selected={}", selected ? selected->GetName() : "(none)");
		lastSelected_ = selected;
	}

	if (Input::GetKeyDown(KeyCode::Escape)) {
		Application::Quit();
	}
}
