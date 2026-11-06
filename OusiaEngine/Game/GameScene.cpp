#include "GameScene.h"
#include "Asset/AssetDatabase.h"
#include "Core/Application.h"
#include "Core/Time.h"
#include "Diagnostics/Log.h"
#include "Math/Rect.h"
#include "Render/Asset/Texture.h"
#include "Render/Camera.h"
#include "Scene/GameObject.h"
#include "UI/Asset/Sprite.h"
#include "UI/Button.h"
#include "UI/Canvas.h"
#include "UI/EventSystem/EventSystem.h"
#include "UI/EventSystem/StandaloneInputModule.h"
#include "UI/GraphicRaycaster.h"
#include "UI/Image.h"
#include "UI/RectTransform.h"
#include "UI/Text.h"

#include <cmath>
#include <format>

using namespace ou;


namespace {

int sFailCount = 0;

void Check(const char *name, bool ok) {
	if (!ok) { sFailCount++; }
	Log::Info("TEST {} {}", ok ? "OK  " : "FAIL", name);
}

bool Near(float a, float b) {
	return std::abs(a - b) < 1e-4f;
}

bool Near(const Vector2 &a, const Vector2 &b) {
	return Near(a.x, b.x) && Near(a.y, b.y);
}

void RunRectTests() {
	const Rect r(10.0f, 20.0f, 100.0f, 50.0f);
	Check("Get: position", Near(r.GetPosition(), {10.0f, 20.0f}));
	Check("Get: size", Near(r.GetSize(), {100.0f, 50.0f}));
	Check("Get: center", Near(r.GetCenter(), {60.0f, 45.0f}));
	Check("Get: min", Near(r.GetMin(), {10.0f, 20.0f}));
	Check("Get: max", Near(r.GetMax(), {110.0f, 70.0f}));

	auto a = r;
	a.SetXMin(30.0f);
	Check("SetXMin: xMax は動かない", Near(a.x, 30.0f) && Near(a.width, 80.0f) && Near(a.GetXMax(), 110.0f));
	a = r;
	a.SetYMin(0.0f);
	Check("SetYMin: yMax は動かない", Near(a.y, 0.0f) && Near(a.height, 70.0f) && Near(a.GetYMax(), 70.0f));
	a = r;
	a.SetXMax(50.0f);
	Check("SetXMax: x は動かない", Near(a.x, 10.0f) && Near(a.width, 40.0f));
	a = r;
	a.SetMin({0.0f, 0.0f});
	Check("SetMin: max は動かない", Near(a.GetMax(), {110.0f, 70.0f}) && Near(a.GetSize(), {110.0f, 70.0f}));
	a = r;
	a.SetMax({200.0f, 100.0f});
	Check("SetMax: min は動かない", Near(a.GetMin(), {10.0f, 20.0f}) && Near(a.GetSize(), {190.0f, 80.0f}));
	a = r;
	a.SetCenter({0.0f, 0.0f});
	Check("SetCenter: 大きさは変わらない", Near(a.GetPosition(), {-50.0f, -25.0f}) && Near(a.GetSize(), {100.0f, 50.0f}));
	a = r;
	a.SetPosition({1.0f, 2.0f});
	a.SetSize({3.0f, 4.0f});
	Check("SetPosition / SetSize", a == Rect(1.0f, 2.0f, 3.0f, 4.0f));

	Check("Contains: 中", r.Contains({50.0f, 30.0f}));
	Check("Contains: min の辺は含む", r.Contains({10.0f, 20.0f}));
	Check("Contains: max の辺は含まない", !r.Contains({110.0f, 40.0f}) && !r.Contains({50.0f, 70.0f}));
	Check("Contains: 外", !r.Contains({5.0f, 30.0f}));

	Check("Overlaps: 重なる", r.Overlaps(Rect(100.0f, 60.0f, 50.0f, 50.0f)));
	Check("Overlaps: 辺が接するだけは重ならない", !r.Overlaps(Rect(110.0f, 20.0f, 10.0f, 10.0f)));
	Check("Overlaps: 離れている", !r.Overlaps(Rect(200.0f, 200.0f, 10.0f, 10.0f)));

	Check("MinMaxRect", Rect::MinMaxRect(10.0f, 20.0f, 110.0f, 70.0f) == r);
	Check("Zero", Rect::Zero == Rect() && Rect::Zero == Rect(0.0f, 0.0f, 0.0f, 0.0f));
	Check("!=", r != Rect::Zero);

	Check("NormalizedToPoint", Near(Rect::NormalizedToPoint(r, {0.5f, 1.0f}), {60.0f, 70.0f}));
	Check("NormalizedToPoint: 0..1 に clamp", Near(Rect::NormalizedToPoint(r, {2.0f, -1.0f}), {110.0f, 20.0f}));
	Check("PointToNormalized", Near(Rect::PointToNormalized(r, {35.0f, 45.0f}), {0.25f, 0.5f}));
	Check("PointToNormalized: 0..1 に clamp", Near(Rect::PointToNormalized(r, {-100.0f, 1000.0f}), {0.0f, 1.0f}));

	Log::Info("TEST ToString {}", r.ToString());
	Log::Info("TEST ToString(.1f) {}", r.ToString(".1f"));
	Log::Info("TEST Rect の失敗 {} 件", sFailCount);
}

RectTransform *Place(GameObject *object, Transform *parent, const Vector2 &anchorMin, const Vector2 &anchorMax, const Vector2 &pivot,
					 const Vector2 &position, const Vector2 &size) {
	auto *rect = dynamic_cast<RectTransform *>(object->GetTransform());
	rect->SetParent(parent);
	rect->SetAnchorMin(anchorMin);
	rect->SetAnchorMax(anchorMax);
	rect->SetPivot(pivot);
	rect->SetAnchoredPosition(position);
	rect->SetSizeDelta(size);
	return rect;
}

RectTransform *CreateImage(Scene &scene, const std::string &name, Transform *parent, const Color &color, const Vector2 &anchorMin,
						   const Vector2 &anchorMax, const Vector2 &pivot, const Vector2 &position, const Vector2 &size) {
	auto *object = scene.CreateGameObject(name);
	object->AddComponent<Image>()->SetColor(color);
	return Place(object, parent, anchorMin, anchorMax, pivot, position, size);
}

void CreateLabel(Scene &scene, Transform *parent, const std::string &text, float fontSize, TextAlignment alignment) {
	auto *object = scene.CreateGameObject("Label " + text);
	auto *label = object->AddComponent<Text>();
	label->SetText(text);
	label->SetFontSize(fontSize);
	label->SetAlignment(alignment);
	label->SetRaycastTarget(false);
	// 親いっぱいに広げて、アライメントで置く
	Place(object, parent, Vector2::Zero, Vector2::One, {0.5f, 0.5f}, Vector2::Zero, Vector2::Zero);
}

} // namespace



void ou::BuildGameScene(Scene &scene) {
	RunRectTests();

	scene.CreateGameObject("Main Camera")->AddComponent<Camera>();

	auto *canvasObject = scene.CreateGameObject("Canvas");
	canvasObject->AddComponent<Canvas>();
	canvasObject->AddComponent<GraphicRaycaster>();
	auto *canvas = canvasObject->GetTransform();

	auto *eventSystem = scene.CreateGameObject("EventSystem");
	eventSystem->AddComponent<EventSystem>();
	eventSystem->AddComponent<StandaloneInputModule>();

	auto *checker = scene.CreateGameObject("Checker")->AddComponent<RectLayoutChecker>();
	auto add = [&](const std::string &name, RectTransform *rect) {
		checker->targets.emplace_back(name, rect);
		return rect;
	};

	// 全体に広げた背景（stretch）
	add("Background", CreateImage(scene, "Background", canvas, {0.15f, 0.15f, 0.18f, 1.0f}, Vector2::Zero, Vector2::One, {0.5f, 0.5f},
								  Vector2::Zero, Vector2::Zero));

	// 四隅（アンカーとピボットを角にする）
	add("BottomLeft", CreateImage(scene, "BottomLeft", canvas, {0.9f, 0.2f, 0.2f, 1.0f}, {0.0f, 0.0f}, {0.0f, 0.0f}, {0.0f, 0.0f},
								  {20.0f, 20.0f}, {120.0f, 120.0f}));
	add("BottomRight", CreateImage(scene, "BottomRight", canvas, {0.2f, 0.9f, 0.2f, 1.0f}, {1.0f, 0.0f}, {1.0f, 0.0f}, {1.0f, 0.0f},
								   {-20.0f, 20.0f}, {120.0f, 120.0f}));
	add("TopLeft", CreateImage(scene, "TopLeft", canvas, {0.2f, 0.4f, 0.95f, 1.0f}, {0.0f, 1.0f}, {0.0f, 1.0f}, {0.0f, 1.0f},
							   {20.0f, -20.0f}, {120.0f, 120.0f}));
	add("TopRight", CreateImage(scene, "TopRight", canvas, {0.95f, 0.85f, 0.2f, 1.0f}, {1.0f, 1.0f}, {1.0f, 1.0f}, {1.0f, 1.0f},
								{-20.0f, -20.0f}, {120.0f, 120.0f}));

	// 右半分に広げたパネルと、その中の子（親の Rect が子のアンカーの基準になるか）
	auto *panel = add("RightPanel", CreateImage(scene, "RightPanel", canvas, {0.3f, 0.3f, 0.4f, 1.0f}, {0.5f, 0.0f}, {1.0f, 1.0f},
												{0.5f, 0.5f}, Vector2::Zero, {-200.0f, -200.0f}));
	CreateLabel(scene, panel, "右半分（上寄せ）", 36.0f, TextAlignment::Top);
	add("PanelChild", CreateImage(scene, "PanelChild", panel, {0.9f, 0.5f, 0.2f, 1.0f}, {0.0f, 0.0f}, {0.5f, 0.5f}, {0.5f, 0.5f},
								  Vector2::Zero, {-20.0f, -20.0f}));

	// スプライト（UV）
	if (auto texture = AssetDatabase::LoadAssetAtPath<Texture>("Assets/uvChecker.png")) {
		auto *object = scene.CreateGameObject("UvChecker");
		object->AddComponent<Image>()->SetSprite(Sprite::Create(texture));
		add("UvChecker", Place(object, canvas, {0.25f, 0.5f}, {0.25f, 0.5f}, {0.5f, 0.5f}, {0.0f, 60.0f}, {256.0f, 256.0f}));
	}

	// ボタン
	auto *buttonObject = scene.CreateGameObject("Button");
	buttonObject->AddComponent<Image>();
	add("Button", Place(buttonObject, canvas, {0.25f, 0.5f}, {0.25f, 0.5f}, {0.5f, 0.5f}, {0.0f, -160.0f}, {280.0f, 70.0f}));
	auto *button = buttonObject->AddComponent<Button>();
	button->onClick_.AddListener([] { Log::Info("TEST Button onClick"); });
	CreateLabel(scene, buttonObject->GetTransform(), "ボタン", 32.0f, TextAlignment::Center);
}


void ou::RectLayoutChecker::Update() {
	const auto frame = Time::GetFrameCount();
	if (frame == 5) {
		for (const auto &[name, rect] : targets) {
			const auto world = rect->GetLocalToWorldMatrix().MultiplyPoint({rect->GetRect().GetMin(), 0.0f});
			Log::Info("TEST {:12} rect {}  min(canvas) ({:.1f}, {:.1f})", name, rect->GetRect().ToString(), world.x, world.y);
		}
	}
	if (Time::GetTime() > 6.0f) { Application::Quit(); }
}
