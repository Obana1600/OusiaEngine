#include "GameScene.h"
#include "Asset/AssetDatabase.h"
#include "Core/Time.h"
#include "Diagnostics/Log.h"
#include "Input/Input.h"
#include "Math/Mathf.h"
#include "Physics/BoxCollider.h"
#include "Physics/Collision.h"
#include "Physics/Physics.h"
#include "Physics/Rigidbody.h"
#include "Physics/SphereCollider.h"
#include "Scene/GameObject.h"
#include "UI/Asset/FontAsset.h"
#include "UI/Canvas.h"
#include "UI/Image.h"
#include "UI/RectTransform.h"
#include "UI/Text.h"

#include <imgui/imgui.h>
#include <imgui/imgui_stdlib.h>

#include <format>
#include <iterator>

using namespace ou;


namespace {

constexpr const char *kFontWeights[] = {
	"Thin", "ExtraLight", "Light", "Regular", "Medium", "SemiBold", "Bold", "ExtraBold", "Black"};

constexpr const char *kAlignmentNames[] = {
	"TopLeft", "Top", "TopRight", "Left", "Center", "Right", "BottomLeft", "Bottom", "BottomRight"};

struct Preset {
	const char *name;
	const char *text;
};

constexpr Preset kPresets[] = {
	{"日本語", "吾輩は猫である。名前はまだ無い。\nどこで生れたかとんと見当がつかぬ。何でも薄暗いじめじめした所でニャーニャー泣いていた事だけは記憶している。"},
	{"English", "The quick brown fox jumps over the lazy dog. Pack my box with five dozen liquor jugs."},
	{"混在", "Unity と同じ Text を作る。TextMeshPro 風の SDF で、拡大しても縁がなめらか。"},
	{"絵文字・壊れた UTF-8", "絵文字: 😀 / 壊れた UTF-8: \xFF\xFE / ここは出る"},
	{"空", ""},
};

std::shared_ptr<FontAsset> LoadFontWeight(int index) {
	return AssetDatabase::LoadAssetAtPath<FontAsset>(
		std::format("EngineAssets/Font/NotoSansJP-{}.ttf", kFontWeights[index]));
}

RectTransform *PlaceOnCanvas(GameObject *object, Transform *canvas, const Vector2 &position, const Vector2 &size) {
	auto *rect = dynamic_cast<RectTransform *>(object->GetTransform());
	rect->SetParent(canvas);
	rect->SetAnchoredPosition(position);
	rect->SetSizeDelta(size);
	return rect;
}

struct TextBox {
	Image *frame;
	Text *text;
};

// 薄い枠（RectTransform の範囲）と、同じ範囲の Text
TextBox CreateTextBox(Scene &scene, Transform *canvas, std::string_view name, const Vector2 &position, const Vector2 &size) {
	auto *frameObject = scene.CreateGameObject(std::format("{} Frame", name));
	auto *frame = frameObject->AddComponent<Image>();
	frame->SetColor({1.0f, 1.0f, 1.0f, 0.15f});
	PlaceOnCanvas(frameObject, canvas, position, size);

	auto *textObject = scene.CreateGameObject(name);
	auto *text = textObject->AddComponent<Text>();
	PlaceOnCanvas(textObject, canvas, position, size);
	return {frame, text};
}

// 見本: 9 つの揃え・大きさ・間隔・ない文字を並べて、一度に見られるようにする
void CreateSamples(Scene &scene, Transform *canvas) {
	for (int i = 0; i < 9; i++) {
		const Vector2 position = {-680.0f + static_cast<float>(i % 3) * 170.0f, 320.0f - static_cast<float>(i / 3) * 120.0f};
		auto box = CreateTextBox(scene, canvas, kAlignmentNames[i], position, {160.0f, 110.0f});
		box.text->SetText(std::format("揃え\n{}", kAlignmentNames[i]));
		box.text->SetFontSize(20.0f);
		box.text->SetAlignment(static_cast<TextAlignment>(i));
	}

	auto bigBox = CreateTextBox(scene, canvas, "Size 120", {-510.0f, -90.0f}, {500.0f, 150.0f});
	bigBox.text->SetText("永Ag");
	bigBox.text->SetFontSize(120.0f);
	bigBox.text->SetAlignment(TextAlignment::Center);

	bigBox.text->SetOutlineWidth(0.25f);
	bigBox.text->SetOutlineColor({0.8f, 0.1f, 0.2f, 1.0f});

	// 左下: SDF の効果（2 x 2）
	const auto createEffectBox = [&](const char *name, const Vector2 &position, const char *text) {
		auto box = CreateTextBox(scene, canvas, name, position, {245.0f, 105.0f});
		box.text->SetText(text);
		box.text->SetFontSize(40.0f);
		box.text->SetAlignment(TextAlignment::Center);
		return box.text;
	};

	auto *outline = createEffectBox("Outline", {-637.0f, -225.0f}, "縁取り\nOutline");
	outline->SetOutlineWidth(0.4f);
	outline->SetOutlineColor(Color::Black);

	// 一番大きくずらす。隣の文字の欠片が映り込まないかも見る
	auto *shadow = createEffectBox("Underlay", {-383.0f, -225.0f}, "影 Underlay\n右下へ 1");
	shadow->SetUnderlayColor({0.0f, 0.0f, 0.0f, 0.8f});
	shadow->SetUnderlayOffset({1.0f, -1.0f});

	auto *glow = createEffectBox("Soft Underlay", {-637.0f, -340.0f}, "ぼかし\nSoftness");
	glow->SetColor({0.1f, 0.1f, 0.3f, 1.0f});
	glow->SetUnderlayColor({1.0f, 0.9f, 0.3f, 1.0f});
	glow->SetUnderlayDilate(0.3f);
	glow->SetUnderlaySoftness(0.5f);

	auto *dilate = createEffectBox("Face Dilate", {-383.0f, -340.0f}, "太く +0.8\n");
	dilate->SetFaceDilate(0.8f);
	auto *thin = CreateTextBox(scene, canvas, "Face Dilate Thin", {-383.0f, -362.0f}, {245.0f, 50.0f}).text;
	thin->SetText("細く -0.4");
	thin->SetFontSize(40.0f);
	thin->SetAlignment(TextAlignment::Center);
	thin->SetFaceDilate(-0.4f);

	// 右下: 英語の折り返し・ない文字と壊れた UTF-8・回転と拡大
	auto englishBox = CreateTextBox(scene, canvas, "English Wrap", {-30.0f, -225.0f}, {420.0f, 90.0f});
	englishBox.text->SetText(kPresets[1].text);
	englishBox.text->SetFontSize(22.0f);

	auto brokenBox = CreateTextBox(scene, canvas, "Missing", {-30.0f, -310.0f}, {420.0f, 50.0f});
	brokenBox.text->SetText(kPresets[3].text);
	brokenBox.text->SetFontSize(22.0f);
	brokenBox.text->SetColor({1.0f, 0.6f, 0.6f, 1.0f});

	auto rotatedBox = CreateTextBox(scene, canvas, "Rotated", {400.0f, -360.0f}, {220.0f, 40.0f});
	rotatedBox.text->SetText("回転 20° × 1.5");
	rotatedBox.text->SetFontSize(24.0f);
	rotatedBox.text->SetAlignment(TextAlignment::Center);
	for (auto *transform : {rotatedBox.text->GetTransform(), rotatedBox.frame->GetTransform()}) {
		transform->SetLocalEulerAngles({0.0f, 0.0f, 20.0f * Mathf::Deg2Rad});
		transform->SetLocalScale({1.5f, 1.5f, 1.0f});
	}
}

} // namespace



void ou::TextPlayground::Start() {
	editingText_ = kPresets[0].text;
	text_->SetText(editingText_);
	ApplyTransform();
}


void ou::TextPlayground::Update() {
	if (isCounterRunning_) {
		counter_->SetText(std::format("frame {}  time {:.2f}", Time::GetFrameCount(), Time::GetTime()));
	}

#ifdef USE_IMGUI
	ShowTextWindow();
#endif
}


void ou::TextPlayground::SetTargets(Text *text, Image *frame, Text *counter) {
	text_ = text;
	frame_ = frame;
	counter_ = counter;
}


void ou::TextPlayground::ApplyTransform() {
	// 枠も同じ形にして、Text の範囲が見えるようにする
	for (auto *transform : {text_->GetTransform(), frame_->GetTransform()}) {
		transform->SetLocalEulerAngles({0.0f, 0.0f, rotation_ * Mathf::Deg2Rad});
		transform->SetLocalScale({scale_, scale_, 1.0f});
	}
}


#ifdef USE_IMGUI
void ou::TextPlayground::ShowTextWindow() {
	ImGui::SetNextWindowPos({1120.0f, 30.0f}, ImGuiCond_FirstUseEver);
	ImGui::SetNextWindowSize({460.0f, 700.0f}, ImGuiCond_FirstUseEver);
	if (!ImGui::Begin("Text")) {
		ImGui::End();
		return;
	}

	// 文字列
	ImGui::SeparatorText("text");
	for (const auto &preset : kPresets) {
		if (ImGui::Button(preset.name)) {
			editingText_ = preset.text;
		}
		ImGui::SameLine();
	}
	ImGui::NewLine();
	ImGui::InputTextMultiline("##text", &editingText_, {-1.0f, 120.0f});
	text_->SetText(editingText_);

	// フォント
	ImGui::SeparatorText("font");
	if (ImGui::Combo("weight", &fontIndex_, kFontWeights, static_cast<int>(std::size(kFontWeights)))) {
		text_->SetFont(LoadFontWeight(fontIndex_));
	}
	if (const auto &font = text_->GetFont()) {
		ImGui::Text("atlas pages: %u (%u x %u)", font->GetAtlasTextureCount(), font->GetAtlasSize(), font->GetAtlasSize());
	} else {
		ImGui::Text("既定のフォント (Regular)");
	}

	float fontSize = text_->GetFontSize();
	if (ImGui::SliderFloat("fontSize", &fontSize, 4.0f, 240.0f)) {
		text_->SetFontSize(fontSize);
	}

	// 並べ方
	ImGui::SeparatorText("layout");
	int alignment = static_cast<int>(text_->GetAlignment());
	if (ImGui::Combo("alignment", &alignment, kAlignmentNames, static_cast<int>(std::size(kAlignmentNames)))) {
		text_->SetAlignment(static_cast<TextAlignment>(alignment));
	}
	bool wordWrapping = text_->IsWordWrappingEnabled();
	if (ImGui::Checkbox("wordWrapping", &wordWrapping)) {
		text_->SetWordWrappingEnabled(wordWrapping);
	}
	float lineSpacing = text_->GetLineSpacing();
	if (ImGui::SliderFloat("lineSpacing", &lineSpacing, -50.0f, 150.0f)) {
		text_->SetLineSpacing(lineSpacing);
	}
	float characterSpacing = text_->GetCharacterSpacing();
	if (ImGui::SliderFloat("characterSpacing", &characterSpacing, -30.0f, 100.0f)) {
		text_->SetCharacterSpacing(characterSpacing);
	}

	Color color = text_->GetColor();
	if (ImGui::ColorEdit4("color", &color.r)) {
		text_->SetColor(color);
	}

	// SDF の効果（値の範囲は TMP と同じ）
	ImGui::SeparatorText("SDF");
	float faceDilate = text_->GetFaceDilate();
	if (ImGui::SliderFloat("faceDilate", &faceDilate, -1.0f, 1.0f)) {
		text_->SetFaceDilate(faceDilate);
	}
	float outlineWidth = text_->GetOutlineWidth();
	if (ImGui::SliderFloat("outlineWidth", &outlineWidth, 0.0f, 1.0f)) {
		text_->SetOutlineWidth(outlineWidth);
	}
	Color outlineColor = text_->GetOutlineColor();
	if (ImGui::ColorEdit4("outlineColor", &outlineColor.r)) {
		text_->SetOutlineColor(outlineColor);
	}
	Color underlayColor = text_->GetUnderlayColor();
	if (ImGui::ColorEdit4("underlayColor", &underlayColor.r)) {
		text_->SetUnderlayColor(underlayColor);
	}
	if (ImGui::SliderFloat2("underlayOffset", &underlayOffset_.x, -1.0f, 1.0f)) {
		text_->SetUnderlayOffset(underlayOffset_);
	}
	float underlayDilate = text_->GetUnderlayDilate();
	if (ImGui::SliderFloat("underlayDilate", &underlayDilate, -1.0f, 1.0f)) {
		text_->SetUnderlayDilate(underlayDilate);
	}
	float underlaySoftness = text_->GetUnderlaySoftness();
	if (ImGui::SliderFloat("underlaySoftness", &underlaySoftness, 0.0f, 1.0f)) {
		text_->SetUnderlaySoftness(underlaySoftness);
	}

	// RectTransform
	ImGui::SeparatorText("RectTransform");
	auto *textRect = dynamic_cast<RectTransform *>(text_->GetTransform());
	auto *frameRect = dynamic_cast<RectTransform *>(frame_->GetTransform());
	Vector2 position = textRect->GetAnchoredPosition();
	if (ImGui::DragFloat2("anchoredPosition", &position.x)) {
		textRect->SetAnchoredPosition(position);
		frameRect->SetAnchoredPosition(position);
	}
	Vector2 size = textRect->GetSizeDelta();
	if (ImGui::DragFloat2("sizeDelta", &size.x, 1.0f, 0.0f, 2000.0f)) {
		textRect->SetSizeDelta(size);
		frameRect->SetSizeDelta(size);
	}
	const bool isRotationChanged = ImGui::SliderFloat("rotation", &rotation_, -180.0f, 180.0f);
	const bool isScaleChanged = ImGui::SliderFloat("scale", &scale_, 0.1f, 5.0f);
	if (isRotationChanged || isScaleChanged) {
		ApplyTransform();
	}
	bool isFrameVisible = frame_->IsEnabled();
	if (ImGui::Checkbox("frame", &isFrameVisible)) {
		frame_->SetEnabled(isFrameVisible);
	}

	// 毎フレームの書き換え
	ImGui::SeparatorText("counter");
	ImGui::Checkbox("running", &isCounterRunning_);

	ImGui::End();
}
#endif


void ou::BuildGameScene(Scene &scene) {
	auto *canvasObject = scene.CreateGameObject("Canvas");
	canvasObject->AddComponent<Canvas>();
	auto *canvas = canvasObject->GetTransform();

	CreateSamples(scene, canvas);

	// ImGui で触る Text
	auto playgroundBox = CreateTextBox(scene, canvas, "Playground", {130.0f, 60.0f}, {560.0f, 360.0f});
	playgroundBox.text->SetFontSize(32.0f);

	// 左上に固定したカウンター（アンカーの確認も兼ねる）
	auto *counterObject = scene.CreateGameObject("Counter");
	auto *counter = counterObject->AddComponent<Text>();
	auto *counterRect = PlaceOnCanvas(counterObject, canvas, {16.0f, -8.0f}, {600.0f, 40.0f});
	counterRect->SetAnchorMin({0.0f, 1.0f});
	counterRect->SetAnchorMax({0.0f, 1.0f});
	counterRect->SetPivot({0.0f, 1.0f});
	counter->SetFontSize(24.0f);
	counter->SetColor({1.0f, 0.9f, 0.3f, 1.0f});

	auto *playground = scene.CreateGameObject("Text Playground")->AddComponent<TextPlayground>();
	playground->SetTargets(playgroundBox.text, playgroundBox.frame, counter);

	scene.CreateGameObject("Input Playground")->AddComponent<InputPlayground>();
	scene.CreateGameObject("Query Probe")->AddComponent<QueryProbe>();
}


// ----- 入力の確認 -----

namespace {

struct KeyName {
	KeyCode key;
	const char *name;
};

constexpr KeyName kKeyNames[] = {
	{KeyCode::Backspace, "Backspace"}, {KeyCode::Tab, "Tab"}, {KeyCode::Return, "Return"},
	{KeyCode::Pause, "Pause"}, {KeyCode::CapsLock, "CapsLock"}, {KeyCode::Escape, "Escape"},
	{KeyCode::Space, "Space"}, {KeyCode::PageUp, "PageUp"}, {KeyCode::PageDown, "PageDown"},
	{KeyCode::End, "End"}, {KeyCode::Home, "Home"}, {KeyCode::LeftArrow, "LeftArrow"},
	{KeyCode::UpArrow, "UpArrow"}, {KeyCode::RightArrow, "RightArrow"}, {KeyCode::DownArrow, "DownArrow"},
	{KeyCode::Insert, "Insert"}, {KeyCode::Delete, "Delete"},
	{KeyCode::LeftWindows, "LeftWindows"}, {KeyCode::RightWindows, "RightWindows"},
	{KeyCode::KeypadMultiply, "KeypadMultiply"}, {KeyCode::KeypadPlus, "KeypadPlus"},
	{KeyCode::KeypadMinus, "KeypadMinus"}, {KeyCode::KeypadPeriod, "KeypadPeriod"},
	{KeyCode::KeypadDivide, "KeypadDivide"}, {KeyCode::Numlock, "Numlock"}, {KeyCode::ScrollLock, "ScrollLock"},
	{KeyCode::LeftShift, "LeftShift"}, {KeyCode::RightShift, "RightShift"},
	{KeyCode::LeftControl, "LeftControl"}, {KeyCode::RightControl, "RightControl"},
	{KeyCode::LeftAlt, "LeftAlt"}, {KeyCode::RightAlt, "RightAlt"},
};

struct GamepadButtonName {
	GamepadButton button;
	const char *name;
};

constexpr GamepadButtonName kGamepadButtonNames[] = {
	{GamepadButton::DpadUp, "DpadUp"}, {GamepadButton::DpadDown, "DpadDown"},
	{GamepadButton::DpadLeft, "DpadLeft"}, {GamepadButton::DpadRight, "DpadRight"},
	{GamepadButton::Start, "Start"}, {GamepadButton::Select, "Select"},
	{GamepadButton::LeftStick, "LeftStick"}, {GamepadButton::RightStick, "RightStick"},
	{GamepadButton::LeftShoulder, "LeftShoulder"}, {GamepadButton::RightShoulder, "RightShoulder"},
	{GamepadButton::South, "South"}, {GamepadButton::East, "East"},
	{GamepadButton::West, "West"}, {GamepadButton::North, "North"},
};

// KeyCode の名前（連番のものは計算し、表にないものは VK の番号）
std::string FormatKey(uint8_t vk) {
	if ('0' <= vk && vk <= '9') { return std::format("Alpha{}", static_cast<char>(vk)); }
	if ('A' <= vk && vk <= 'Z') { return std::string(1, static_cast<char>(vk)); }
	if (0x60 <= vk && vk <= 0x69) { return std::format("Keypad{}", vk - 0x60); }
	if (0x70 <= vk && vk <= 0x7B) { return std::format("F{}", vk - 0x70 + 1); }
	for (const auto &entry : kKeyNames) {
		if (static_cast<uint8_t>(entry.key) == vk) { return entry.name; }
	}
	return std::format("VK 0x{:02X}", vk);
}

} // namespace


void ou::InputPlayground::Update() {
	const auto frame = Time::GetFrameCount();

	// キー: Down / Up をログに出す
	std::string held;
	for (int i = 1; i < 255; ++i) {
		const auto key = static_cast<KeyCode>(i);
		const auto name = FormatKey(static_cast<uint8_t>(i));
		if (Input::GetKeyDown(key)) {
			Log::Info("TEST frame {} KeyDown {}", frame, name);
			AddEvent(std::format("{} KeyDown {}", frame, name));
		}
		if (Input::GetKeyUp(key)) {
			Log::Info("TEST frame {} KeyUp {}", frame, name);
			AddEvent(std::format("{} KeyUp {}", frame, name));
		}
		if (Input::GetKey(key)) { held += name + " "; }
	}

	// マウスのボタン（範囲外の 5 と -1 は false のはず）
	const auto position = Input::GetMousePosition();
	for (int button = -1; button <= 5; ++button) {
		if (Input::GetMouseButtonDown(button)) {
			Log::Info("TEST frame {} MouseDown {} at ({}, {})", frame, button, position.x, position.y);
			AddEvent(std::format("{} MouseDown {}", frame, button));
		}
		if (Input::GetMouseButtonUp(button)) {
			Log::Info("TEST frame {} MouseUp {} at ({}, {})", frame, button, position.x, position.y);
			AddEvent(std::format("{} MouseUp {}", frame, button));
		}
	}

	// 移動量とホイール: 動いたフレームだけ出す
	const auto delta = Input::GetMouseDelta();
	const auto scroll = Input::GetMouseScrollDelta();
	if (delta.x != 0.0f || delta.y != 0.0f) {
		Log::Info("TEST frame {} MouseDelta ({}, {}) position ({}, {})", frame, delta.x, delta.y, position.x, position.y);
	}
	if (scroll.x != 0.0f || scroll.y != 0.0f) {
		Log::Info("TEST frame {} Scroll ({}, {})", frame, scroll.x, scroll.y);
	}
	deltaTotal_ = {deltaTotal_.x + delta.x, deltaTotal_.y + delta.y};
	scrollTotal_ = {scrollTotal_.x + scroll.x, scrollTotal_.y + scroll.y};

	UpdateGamepads(frame);

#ifdef USE_IMGUI
	ImGui::SetNextWindowPos({920.0f, 20.0f}, ImGuiCond_FirstUseEver);
	ImGui::SetNextWindowSize({340.0f, 420.0f}, ImGuiCond_FirstUseEver);
	if (ImGui::Begin("Input")) {
		ImGui::TextWrapped("Held: %s", held.c_str());
		ImGui::Text("Mouse buttons: %d %d %d %d %d",
			Input::GetMouseButton(0), Input::GetMouseButton(1), Input::GetMouseButton(2),
			Input::GetMouseButton(3), Input::GetMouseButton(4));
		ImGui::Text("Position: (%.0f, %.0f)", position.x, position.y);
		ImGui::Text("Delta: (%.0f, %.0f)  total (%.0f, %.0f)", delta.x, delta.y, deltaTotal_.x, deltaTotal_.y);
		ImGui::Text("Scroll total: (%.1f, %.1f)", scrollTotal_.x, scrollTotal_.y);
		ImGui::Separator();
		for (int player = 0; player < 4; ++player) {
			if (!Input::IsGamepadConnected(player)) {
				ImGui::Text("Pad %d: not connected", player);
				continue;
			}
			std::string buttons;
			for (const auto &entry : kGamepadButtonNames) {
				if (Input::GetGamepadButton(entry.button, player)) { buttons += std::string(entry.name) + " "; }
			}
			const auto left = Input::GetGamepadLeftStick(player);
			const auto right = Input::GetGamepadRightStick(player);
			ImGui::Text("Pad %d: L (%.2f, %.2f)  R (%.2f, %.2f)", player, left.x, left.y, right.x, right.y);
			ImGui::Text("  Trigger L %.2f  R %.2f", Input::GetGamepadLeftTrigger(player), Input::GetGamepadRightTrigger(player));
			ImGui::TextWrapped("  Buttons: %s", buttons.c_str());
		}
		ImGui::SliderFloat("Pad 0 low motor", &lowMotor_, 0.0f, 1.0f);
		ImGui::SliderFloat("Pad 0 high motor", &highMotor_, 0.0f, 1.0f);
		ImGui::Separator();
		for (auto it = events_.rbegin(); it != events_.rend(); ++it) {
			ImGui::TextUnformatted(it->c_str());
		}
	}
	ImGui::End();
#endif
}


void ou::InputPlayground::UpdateGamepads(uint64_t frame) {
	// 範囲外の player は、つながっていない・0 と同じに返る
	if (!hasCheckedRange_) {
		hasCheckedRange_ = true;
		for (int player : {-1, 4, 100}) {
			const auto stick = Input::GetGamepadLeftStick(player);
			Log::Info("TEST player {} connected {} button {} down {} stick ({}, {}) trigger {}",
				player, Input::IsGamepadConnected(player), Input::GetGamepadButton(GamepadButton::South, player),
				Input::GetGamepadButtonDown(GamepadButton::South, player), stick.x, stick.y,
				Input::GetGamepadRightTrigger(player));
			Input::SetGamepadMotorSpeeds(1.0f, 1.0f, player); // 落ちないこと
		}
	}

	for (int player = 0; player < 4; ++player) {
		const bool isConnected = Input::IsGamepadConnected(player);
		if (isConnected != wasConnected_[player]) {
			Log::Info("TEST frame {} Pad {} connected {}", frame, player, isConnected);
			wasConnected_[player] = isConnected;
		}

		for (const auto &entry : kGamepadButtonNames) {
			if (Input::GetGamepadButtonDown(entry.button, player)) {
				Log::Info("TEST frame {} Pad {} ButtonDown {}", frame, player, entry.name);
				AddEvent(std::format("{} Pad {} Down {}", frame, player, entry.name));
			}
			if (Input::GetGamepadButtonUp(entry.button, player)) {
				Log::Info("TEST frame {} Pad {} ButtonUp {}", frame, player, entry.name);
				AddEvent(std::format("{} Pad {} Up {}", frame, player, entry.name));
			}
		}
	}

	Input::SetGamepadMotorSpeeds(lowMotor_, highMotor_, 0);
}


void ou::InputPlayground::AddEvent(std::string text) {
	constexpr size_t kMaxEvents = 16;
	events_.push_back(std::move(text));
	if (events_.size() > kMaxEvents) { events_.erase(events_.begin()); }
}


// ----- FixedUpdate の確認 -----

void ou::FixedUpdateProbe::Start() {
	Log::Info("TEST frame {} Start", Time::GetFrameCount());
	ChangePhase(0);
}


void ou::FixedUpdateProbe::FixedUpdate() {
	++fixedCount_;
	++frameFixedCount_;

	// 最初の数フレームだけ、順番と FixedUpdate の中の時間を出す
	if (Time::GetFrameCount() <= 4) {
		Log::Info("TEST frame {} FixedUpdate deltaTime {} time {:.4f} fixedTime {:.4f} inFixed {}",
			Time::GetFrameCount(), Time::GetDeltaTime(), Time::GetTime(), Time::GetFixedTime(), Time::IsInFixedTimeStep());
	}
}


void ou::FixedUpdateProbe::Update() {
	if (Time::GetFrameCount() <= 4) {
		Log::Info("TEST frame {} Update deltaTime {} time {:.4f} inFixed {} fixedInFrame {}",
			Time::GetFrameCount(), Time::GetDeltaTime(), Time::GetTime(), Time::IsInFixedTimeStep(), frameFixedCount_);
	}

	// 1 秒ごとに、その 1 秒の回数を出す
	realTime_ += Time::GetUnscaledDeltaTime();
	secondTimer_ += Time::GetUnscaledDeltaTime();
	if (secondTimer_ >= 1.0f) {
		Log::Info("TEST phase {} fixedDeltaTime {} timeScale {} -> FixedUpdate {} / {:.2f}s, max {} per frame",
			phase_, Time::GetFixedDeltaTime(), Time::GetTimeScale(), fixedCount_, secondTimer_, maxFixedPerFrame_);
		secondTimer_ = 0.0f;
		fixedCount_ = 0;
		maxFixedPerFrame_ = 0;
	}

	// 3 秒ごとに設定を変える
	const int phase = static_cast<int>(realTime_ / 3.0f);
	if (phase != phase_) { ChangePhase(phase); }
}


void ou::FixedUpdateProbe::LateUpdate() {
	if (Time::GetFrameCount() <= 4) {
		Log::Info("TEST frame {} LateUpdate", Time::GetFrameCount());
	}
	if (frameFixedCount_ > maxFixedPerFrame_) { maxFixedPerFrame_ = frameFixedCount_; }
	frameFixedCount_ = 0;
}


void ou::FixedUpdateProbe::ChangePhase(int phase) {
	phase_ = phase;
	switch (phase) {
		case 0: // 既定
			Time::SetFixedDeltaTime(0.02f);
			Time::SetTimeScale(1.0f);
			break;
		case 1: // 1 秒に 120 回
			Time::SetFixedDeltaTime(1.0f / 120.0f);
			break;
		case 2: // timeScale 0.5 → 25 回
			Time::SetFixedDeltaTime(0.02f);
			Time::SetTimeScale(0.5f);
			break;
		case 3: // timeScale 0 → 0 回
			Time::SetTimeScale(0.0f);
			break;
		case 4: // 0 を入れても下限で止まらない
			Time::SetTimeScale(1.0f);
			Time::SetFixedDeltaTime(0.0f);
			break;
		default: // 元に戻す
			Time::SetFixedDeltaTime(0.02f);
			Time::SetTimeScale(1.0f);
			break;
	}
	Log::Info("TEST phase {} start: fixedDeltaTime {} timeScale {}", phase_, Time::GetFixedDeltaTime(), Time::GetTimeScale());
}


// ----- 調べものの確認 -----

namespace {

std::string FormatVector(const Vector3 &v) {
	return std::format("({:.3f}, {:.3f}, {:.3f})", v.x, v.y, v.z);
}

} // namespace


BoxCollider *ou::QueryProbe::CreateBox(const char *name, const Vector3 &position, const Vector3 &size, Transform *parent) {
	auto *object = GetGameObject()->GetScene()->CreateGameObject(name);
	object->GetTransform()->SetParent(parent);
	object->GetTransform()->SetLocalPosition(position);
	auto *box = object->AddComponent<BoxCollider>();
	box->SetSize(size);
	return box;
}


void ou::QueryProbe::Start() {
	// x = 0 の柱: 上から Trigger（上の面 7.1）・BoxB（4.5）・BoxA（2.5）・Floor（0.5）
	CreateBox("Floor", Vector3::Zero, {40.0f, 1.0f, 40.0f});
	CreateBox("BoxA", {0.0f, 2.0f, 0.0f}, Vector3::One);
	CreateBox("BoxB", {0.0f, 4.0f, 0.0f}, Vector3::One);
	CreateBox("Trigger", {0.0f, 7.0f, 0.0f}, {2.0f, 0.2f, 2.0f})->SetIsTrigger(true);

	// Rigidbody（重力なし）の子のコライダー: GetTransform は Parent を返す
	auto *parent = GetGameObject()->GetScene()->CreateGameObject("Parent");
	parent->GetTransform()->SetLocalPosition({5.0f, 3.0f, 0.0f});
	parent->AddComponent<Rigidbody>()->SetUseGravity(false);
	CreateBox("ChildBox", Vector3::Zero, Vector3::One, parent->GetTransform());

	// 2 歩目に無効にする: 当たらなくなる
	disabled_ = CreateBox("Disabled", {10.0f, 2.0f, 0.0f}, Vector3::One);
}


void ou::QueryProbe::FixedUpdate() {
	step_++;

	// 形は 1 歩目の SyncTransform で作られる
	if (step_ == 2) { disabled_->SetEnabled(false); }
	if (step_ != 3) { return; }

	const Vector3 down = {0.0f, -1.0f, 0.0f};

	// 1 本目の当たり
	RaycastHit hit;
	const auto isHit = Physics::Raycast({0.0f, 10.0f, 0.0f}, down * 5.0f, &hit); // 長さ 5 の向きでもよい
	Log::Info("TEST Raycast down: hit {} collider {} point {} normal {} distance {:.3f}", isHit,
		hit.collider ? hit.collider->GetGameObject()->GetName() : "none", FormatVector(hit.point), FormatVector(hit.normal), hit.distance);

	// 近い順・トリガー・届く距離
	LogRaycastAll("all UseGlobal", {0.0f, 10.0f, 0.0f}, down, Mathf::Infinity);
	LogRaycastAll("all Ignore", {0.0f, 10.0f, 0.0f}, down, Mathf::Infinity, QueryTriggerInteracation::Ignore);
	LogRaycastAll("all Collide", {0.0f, 10.0f, 0.0f}, down, Mathf::Infinity, QueryTriggerInteracation::Collide);
	Physics::SetQueriesHitTriggers(false);
	LogRaycastAll("all UseGlobal (queriesHitTriggers false)", {0.0f, 10.0f, 0.0f}, down, Mathf::Infinity);
	Physics::SetQueriesHitTriggers(true);
	LogRaycastAll("maxDistance 3 Ignore", {0.0f, 10.0f, 0.0f}, down, 3.0f, QueryTriggerInteracation::Ignore);
	LogRaycastAll("maxDistance 6 Ignore", {0.0f, 10.0f, 0.0f}, down, 6.0f, QueryTriggerInteracation::Ignore);

	// 始点が BoxB の中: BoxB には当たらず BoxA から
	LogRaycastAll("inside BoxB", {0.0f, 4.0f, 0.0f}, down, Mathf::Infinity);

	// 横から: BoxA の左の面（x = -0.5）、法線 (-1, 0, 0)、距離 4.5
	LogRaycastAll("side", {-5.0f, 2.0f, 0.0f}, {1.0f, 0.0f, 0.0f}, Mathf::Infinity);

	// Rigidbody の子: transform は Parent
	if (Physics::Raycast({5.0f, 10.0f, 0.0f}, down, &hit)) {
		Log::Info("TEST Raycast child: collider {} rigidbody {} transform {}", hit.collider->GetGameObject()->GetName(),
			hit.GetRigidbody() != nullptr, hit.GetTransform()->GetGameObject()->GetName());
	}

	// 無効にしたコライダー: Floor にだけ当たる
	LogRaycastAll("disabled", {10.0f, 10.0f, 0.0f}, down, Mathf::Infinity);

	// 重なり
	LogColliders("OverlapSphere (0, 2, 0) r 0.6", Physics::OverlapSphere({0.0f, 2.0f, 0.0f}, 0.6f));
	LogColliders("OverlapSphere (0, 3, 0) r 0.6", Physics::OverlapSphere({0.0f, 3.0f, 0.0f}, 0.6f));
	LogColliders("OverlapSphere trigger Ignore", Physics::OverlapSphere({0.0f, 7.0f, 0.0f}, 0.5f, QueryTriggerInteracation::Ignore));
	LogColliders("OverlapSphere trigger Collide", Physics::OverlapSphere({0.0f, 7.0f, 0.0f}, 0.5f, QueryTriggerInteracation::Collide));
	LogColliders("OverlapBox long z", Physics::OverlapBox({1.2f, 2.0f, 0.0f}, {0.05f, 0.05f, 1.0f}));
	LogColliders("OverlapBox long z rotated 90 around y", Physics::OverlapBox({1.2f, 2.0f, 0.0f}, {0.05f, 0.05f, 1.0f},
		Quaternion::AngleAxis(Vector3::Up, 90.0f * Mathf::Deg2Rad)));
}


void ou::QueryProbe::LogRaycastAll(const char *label, const Vector3 &origin, const Vector3 &direction, float maxDistance,
								   QueryTriggerInteracation queryTriggerInteraction) {
	std::string text;
	for (const auto &hit : Physics::RaycastAll(origin, direction, maxDistance, queryTriggerInteraction)) {
		text += std::format(" [{} point {} normal {} distance {:.3f}]", hit.collider->GetGameObject()->GetName(),
			FormatVector(hit.point), FormatVector(hit.normal), hit.distance);
	}
	Log::Info("TEST RaycastAll {}:{}", label, text.empty() ? " none" : text);
}


void ou::QueryProbe::LogColliders(const char *label, const std::vector<Collider *> &colliders) {
	std::string text;
	for (const auto *collider : colliders) {
		text += " " + collider->GetGameObject()->GetName();
	}
	Log::Info("TEST {}:{}", label, text.empty() ? " none" : text);
}
