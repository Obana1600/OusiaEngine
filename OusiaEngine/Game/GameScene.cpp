#include "GameScene.h"
#include "Asset/AssetDatabase.h"
#include "Core/Time.h"
#include "Math/Mathf.h"
#include "Scene/GameObject.h"
#include "UI/Canvas.h"
#include "UI/FontAsset.h"
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

	auto smallBox = CreateTextBox(scene, canvas, "Size 12", {-510.0f, -215.0f}, {500.0f, 80.0f});
	smallBox.text->SetText("12 px: The quick brown fox jumps over the lazy dog. 吾輩は猫である。名前はまだ無い。どこで生れたかとんと見当がつかぬ。");
	smallBox.text->SetFontSize(12.0f);

	auto spacingBox = CreateTextBox(scene, canvas, "Spacing", {-510.0f, -330.0f}, {500.0f, 130.0f});
	spacingBox.text->SetText("行間 50・文字間 20\nline 2");
	spacingBox.text->SetFontSize(24.0f);
	spacingBox.text->SetLineSpacing(50.0f);
	spacingBox.text->SetCharacterSpacing(20.0f);

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
}
