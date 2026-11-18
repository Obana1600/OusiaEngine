#include "ProjectSettingsWindow.h"
#include "Core/PlayerSettings.h"
#include "Core/TimeManager.h"
#include "Diagnostics/Log.h"
#include "Editor/EditorGuiLayout.h"
#include "Editor/GuiContent.h"
#include "Physics/Physics.h"
#include "Scene/TagManager.h"

#include <algorithm>
#include <cfloat>
#include <string_view>
#include <utility>
#include <vector>

#include <imgui/imgui.h>
#include <imgui/imgui_internal.h>
#include <imgui/imgui_stdlib.h>

using namespace ou;


namespace {

struct SectionInfo {
	ProjectSettingsWindow::Section section;
	const char *name;
};

// Unity と同じく、名前の順に並べる
constexpr SectionInfo kSections[] = {
	{ProjectSettingsWindow::Section::Physics, "Physics"},
	{ProjectSettingsWindow::Section::Player, "Player"},
	{ProjectSettingsWindow::Section::TagsAndLayers, "Tags and Layers"},
	{ProjectSettingsWindow::Section::Time, "Time"},
};

// 前と後ろの空白を取る
std::string Trim(std::string_view text) {
	const auto first = text.find_first_not_of(" \t");
	if (first == std::string_view::npos) { return {}; }
	const auto last = text.find_last_not_of(" \t");
	return std::string(text.substr(first, last - first + 1));
}

// 下から上へ読む向き（左に 90 度回した向き）で文字を描く。pos は文字の左下になる点
void AddVerticalText(ImDrawList *drawList, const ImVec2 &pos, ImU32 color, const char *text) {
	const auto start = drawList->VtxBuffer.Size;
	drawList->AddText(pos, color, text);
	for (auto i = start; i < drawList->VtxBuffer.Size; i++) {
		auto &vertex = drawList->VtxBuffer[i];
		const auto dx = vertex.pos.x - pos.x;
		const auto dy = vertex.pos.y - pos.y;
		vertex.pos = {pos.x + dy, pos.y - dx};
	}
}

// 欄から出たときに保存する（DragFloat など、動かしている間は毎フレーム値だけ入れる）
bool IsEditFinished() {
	return ImGui::IsItemDeactivatedAfterEdit();
}

} // namespace



void ou::ProjectSettingsWindow::Open(Section section) {
	GetWindow<ProjectSettingsWindow>()->section_ = section;
}


ou::ProjectSettingsWindow::ProjectSettingsWindow() {
	SetTitle("Project Settings");
}


void ou::ProjectSettingsWindow::OnGui() {
	DrawSectionList();
	ImGui::SameLine();

	ImGui::BeginChild("##Content", {0.0f, 0.0f}, ImGuiChildFlags_None);
	switch (section_) {
		case Section::Physics:
			DrawPhysics();
			break;
		case Section::Player:
			DrawPlayer();
			break;
		case Section::TagsAndLayers:
			DrawTagsAndLayers();
			break;
		case Section::Time:
			DrawTime();
			break;
	}
	ImGui::EndChild();
}


void ou::ProjectSettingsWindow::DrawSectionList() {
	ImGui::BeginChild("##Sections", {ImGui::GetFontSize() * 9.0f, 0.0f}, ImGuiChildFlags_Borders | ImGuiChildFlags_ResizeX);
	for (const auto &info : kSections) {
		if (ImGui::Selectable(info.name, section_ == info.section)) { section_ = info.section; }
	}
	ImGui::EndChild();
}


void ou::ProjectSettingsWindow::DrawPhysics() {
	ImGui::SeparatorText("Physics");

	const auto gravity = Physics::GetGravity();
	float values[3] = {gravity.x, gravity.y, gravity.z};
	EditorGuiLayout::PrefixLabel(GuiContent("Gravity", "Rigidbody にかかる重力 (m/s^2)"));
	ImGui::SetNextItemWidth(-FLT_MIN);
	if (ImGui::DragFloat3("##Gravity", values, 0.1f)) {
		Physics::SetGravity({values[0], values[1], values[2]});
	}
	if (IsEditFinished()) { Physics::SaveDynamicsManager(); }

	auto queriesHitTriggers = Physics::GetQueriesHitTriggers();
	EditorGuiLayout::PrefixLabel(GuiContent("Queries Hit Triggers", "Raycast などが、トリガーの Collider にも当たるか"));
	if (ImGui::Checkbox("##QueriesHitTriggers", &queriesHitTriggers)) {
		Physics::SetQueriesHitTriggers(queriesHitTriggers);
		Physics::SaveDynamicsManager();
	}

	ImGui::Spacing();
	DrawLayerCollisionMatrix();
}


void ou::ProjectSettingsWindow::DrawLayerCollisionMatrix() {
	ImGui::SeparatorText("Layer Collision Matrix");

	// 名前のあるレイヤーだけを出す（Unity と同じ）
	std::vector<int> layers;
	std::vector<std::string> names;
	auto labelWidth = 0.0f;
	auto headerHeight = 0.0f;
	for (int layer = 0; layer < LayerMask::kLayerCount; layer++) {
		auto name = LayerMask::LayerToName(layer);
		if (name.empty()) { continue; }
		const auto size = ImGui::CalcTextSize(name.c_str()).x;
		labelWidth = std::max(labelWidth, size);
		headerHeight = std::max(headerHeight, size);
		layers.push_back(layer);
		names.push_back(std::move(name));
	}

	const auto &style = ImGui::GetStyle();
	const auto count = static_cast<int>(layers.size());
	const auto cell = ImGui::GetFrameHeight() + 2.0f;
	labelWidth += style.ItemSpacing.x * 2.0f;
	headerHeight += style.ItemSpacing.y * 2.0f;

	// 上の見出し：列は後ろのレイヤーから並べる（左上が三角になるように）
	const auto origin = ImGui::GetCursorScreenPos();
	auto *drawList = ImGui::GetWindowDrawList();
	const auto textColor = ImGui::GetColorU32(ImGuiCol_Text);
	const auto lineHeight = ImGui::GetTextLineHeight();
	for (int column = 0; column < count; column++) {
		const auto &name = names[count - 1 - column];
		const ImVec2 pos = {origin.x + labelWidth + column * cell + (cell - lineHeight) * 0.5f, origin.y + headerHeight - style.ItemSpacing.y};
		AddVerticalText(drawList, pos, textColor, name.c_str());
	}
	ImGui::Dummy({labelWidth + count * cell, headerHeight});

	// 行：前のレイヤーから。row と column のレイヤーが当たるかのチェックボックス
	for (int row = 0; row < count; row++) {
		const auto rowY = ImGui::GetCursorScreenPos().y;
		const auto &rowName = names[row];
		const auto textWidth = ImGui::CalcTextSize(rowName.c_str()).x;
		drawList->AddText({origin.x + labelWidth - style.ItemSpacing.x - textWidth, rowY + (cell - lineHeight) * 0.5f}, textColor, rowName.c_str());

		ImGui::PushID(layers[row]);
		for (int column = 0; column < count - row; column++) {
			const auto other = count - 1 - column;
			ImGui::SetCursorScreenPos({origin.x + labelWidth + column * cell + 1.0f, rowY + 1.0f});
			ImGui::PushID(layers[other]);

			auto collides = !Physics::GetIgnoreLayerCollision(layers[row], layers[other]);
			if (ImGui::Checkbox("##Collides", &collides)) {
				Physics::IgnoreLayerCollision(layers[row], layers[other], !collides);
				Physics::SaveDynamicsManager();
			}
			if (ImGui::IsItemHovered()) {
				ImGui::SetTooltip("%s / %s", rowName.c_str(), names[other].c_str());
			}

			ImGui::PopID();
		}
		ImGui::PopID();

		ImGui::SetCursorScreenPos({origin.x, rowY});
		ImGui::Dummy({labelWidth + count * cell, cell});
	}

	ImGui::Spacing();
	if (ImGui::Button("Disable All")) {
		for (auto layer1 : layers) {
			for (auto layer2 : layers) { Physics::IgnoreLayerCollision(layer1, layer2, true); }
		}
		Physics::SaveDynamicsManager();
	}
	ImGui::SameLine();
	if (ImGui::Button("Enable All")) {
		for (auto layer1 : layers) {
			for (auto layer2 : layers) { Physics::IgnoreLayerCollision(layer1, layer2, false); }
		}
		Physics::SaveDynamicsManager();
	}
}


void ou::ProjectSettingsWindow::DrawPlayer() {
	ImGui::SeparatorText("Player");

	// 書き出す exe の名前・ウィンドウのタイトル
	if (ImGui::GetActiveID() != ImGui::GetID("##ProductName")) { productName_ = PlayerSettings::GetProductName(); }
	EditorGuiLayout::PrefixLabel(GuiContent("Product Name", "書き出す exe の名前・ウィンドウのタイトル"));
	ImGui::SetNextItemWidth(-FLT_MIN);
	ImGui::InputText("##ProductName", &productName_);
	if (IsEditFinished()) {
		const auto name = Trim(productName_);
		if (!name.empty()) { PlayerSettings::SetProductName(name); }
	}

	ImGui::SeparatorText("Resolution and Presentation");

	// 書き出した Player の起動のしかた
	constexpr FullScreenMode kModes[] = {FullScreenMode::FullScreenWindow, FullScreenMode::MaximizedWindow, FullScreenMode::Windowed};
	constexpr const char *kModeNames[] = {"Fullscreen Window", "Maximized Window", "Windowed"};

	const auto current = PlayerSettings::GetFullScreenMode();
	int modeIndex = 0;
	for (int i = 0; i < 3; i++) {
		if (kModes[i] == current) { modeIndex = i; }
	}
	EditorGuiLayout::PrefixLabel("Fullscreen Mode");
	ImGui::SetNextItemWidth(-FLT_MIN);
	if (ImGui::Combo("##FullscreenMode", &modeIndex, kModeNames, 3)) {
		PlayerSettings::SetFullScreenMode(kModes[modeIndex]);
	}

	// Windowed で起動するときの大きさ（ほかのモードでもウィンドウに戻したときに使う）
	if (ImGui::GetActiveID() != ImGui::GetID("##DefaultScreenSize")) {
		screenSize_[0] = static_cast<int>(PlayerSettings::GetDefaultScreenWidth());
		screenSize_[1] = static_cast<int>(PlayerSettings::GetDefaultScreenHeight());
	}
	EditorGuiLayout::PrefixLabel(GuiContent("Default Screen Size", "Windowed で起動するときの大きさ"));
	ImGui::SetNextItemWidth(-FLT_MIN);
	ImGui::InputInt2("##DefaultScreenSize", screenSize_);
	if (IsEditFinished()) {
		screenSize_[0] = std::clamp(screenSize_[0], 64, 16384);
		screenSize_[1] = std::clamp(screenSize_[1], 64, 16384);
		PlayerSettings::SetDefaultScreenSize(static_cast<uint32_t>(screenSize_[0]), static_cast<uint32_t>(screenSize_[1]));
	}
}


void ou::ProjectSettingsWindow::DrawTagsAndLayers() {
	DrawTags();
	ImGui::Spacing();
	DrawLayers();
}


void ou::ProjectSettingsWindow::DrawTags() {
	ImGui::SeparatorText("Tags");

	// 組み込みのタグは消せない。Unity と同じく、名前は変えられない（足すか消すだけ）
	std::string removing;
	const auto &tags = TagManager::GetTags();
	const auto buttonWidth = ImGui::GetFrameHeight();
	for (size_t i = 0; i < tags.size(); i++) {
		const auto &tag = tags[i];
		ImGui::PushID(static_cast<int>(i));
		const auto isBuiltin = TagManager::IsBuiltinTag(tag);
		EditorGuiLayout::PrefixLabel(std::string(isBuiltin ? "Builtin Tag " : "Tag ") + std::to_string(i));

		ImGui::AlignTextToFramePadding();
		if (isBuiltin) {
			ImGui::TextDisabled("%s", tag.c_str());
		} else {
			ImGui::TextUnformatted(tag.c_str());
			ImGui::SameLine(ImGui::GetContentRegionMax().x - buttonWidth);
			if (ImGui::Button("-", {buttonWidth, 0.0f})) { removing = tag; }
			if (ImGui::IsItemHovered()) { ImGui::SetTooltip("消す（このタグを付けている GameObject は、タグの文字が残ります）"); }
		}
		ImGui::PopID();
	}
	if (!removing.empty()) { TagManager::RemoveTag(removing); }

	// 足す（Enter か + で）
	EditorGuiLayout::PrefixLabel("New Tag");
	ImGui::SetNextItemWidth(-(buttonWidth + ImGui::GetStyle().ItemSpacing.x));
	const auto entered = ImGui::InputTextWithHint("##NewTag", "タグの名前", &newTag_, ImGuiInputTextFlags_EnterReturnsTrue);
	ImGui::SameLine();
	if (ImGui::Button("+", {buttonWidth, 0.0f}) || entered) {
		tagError_ = TagManager::AddTag(newTag_);
		if (tagError_.empty()) { newTag_.clear(); }
	}
	if (!tagError_.empty()) {
		ImGui::TextColored({1.0f, 0.8f, 0.2f, 1.0f}, "%s", tagError_.c_str());
	}
}


void ou::ProjectSettingsWindow::DrawLayers() {
	ImGui::SeparatorText("Layers");

	for (int layer = 0; layer < LayerMask::kLayerCount; layer++) {
		ImGui::PushID(layer);
		const auto isBuiltin = LayerMask::IsBuiltinLayer(layer);
		const auto label = std::string(isBuiltin ? "Builtin Layer " : "User Layer ") + std::to_string(layer);
		EditorGuiLayout::PrefixLabel(label);
		ImGui::SetNextItemWidth(-FLT_MIN);

		// 打っている間は、打っている文字を残す
		if (ImGui::GetActiveID() != ImGui::GetID("##Name")) { layerNames_[layer] = LayerMask::LayerToName(layer); }

		ImGui::BeginDisabled(isBuiltin);
		ImGui::InputText("##Name", &layerNames_[layer]);
		ImGui::EndDisabled();

		if (!isBuiltin && IsEditFinished()) {
			const auto name = Trim(layerNames_[layer]);
			const auto oldName = LayerMask::LayerToName(layer);
			if (name != oldName) {
				if (LayerMask::SetLayerName(layer, name)) {
					// 名前をなくしたレイヤーは、当たらない組から外す（名前でしか書けないため）
					if (name.empty()) {
						for (int other = 0; other < LayerMask::kLayerCount; other++) {
							Physics::IgnoreLayerCollision(layer, other, false);
						}
					}
					LayerMask::SaveLayerNames();
					// 当たらない組はレイヤーの名前で書いているので、新しい名前で書き直す
					Physics::SaveDynamicsManager();
				} else {
					Log::Warning("レイヤーの名前 \"{}\" は、ほかのレイヤーで使っています", name);
				}
			}
		}
		ImGui::PopID();
	}
}


void ou::ProjectSettingsWindow::DrawTime() {
	ImGui::SeparatorText("Time");

	auto fixedTimestep = TimeManager::GetFixedTimestep();
	EditorGuiLayout::PrefixLabel(GuiContent("Fixed Timestep", "FixedUpdate と物理を動かす間隔 (秒)。Time.fixedDeltaTime の初めの値"));
	ImGui::SetNextItemWidth(-FLT_MIN);
	if (ImGui::DragFloat("##FixedTimestep", &fixedTimestep, 0.0005f, 0.0001f, 1.0f, "%.4f", ImGuiSliderFlags_AlwaysClamp)) {
		TimeManager::SetFixedTimestep(fixedTimestep);
	}
	if (IsEditFinished()) { TimeManager::Save(); }

	auto maximumAllowedTimestep = TimeManager::GetMaximumAllowedTimestep();
	EditorGuiLayout::PrefixLabel(GuiContent("Maximum Allowed Timestep", "1 フレームで進める時間の上限 (秒)。重いフレームのあとに FixedUpdate が何度も続かないようにする"));
	ImGui::SetNextItemWidth(-FLT_MIN);
	if (ImGui::DragFloat("##MaximumAllowedTimestep", &maximumAllowedTimestep, 0.005f, 0.0001f, 1.0f, "%.4f", ImGuiSliderFlags_AlwaysClamp)) {
		TimeManager::SetMaximumAllowedTimestep(maximumAllowedTimestep);
	}
	if (IsEditFinished()) { TimeManager::Save(); }

	auto timeScale = TimeManager::GetTimeScale();
	EditorGuiLayout::PrefixLabel(GuiContent("Time Scale", "時間の進む速さ。Time.timeScale の初めの値（1 で等速、0 で止まる）"));
	ImGui::SetNextItemWidth(-FLT_MIN);
	if (ImGui::DragFloat("##TimeScale", &timeScale, 0.01f, 0.0f, 100.0f, "%.2f", ImGuiSliderFlags_AlwaysClamp)) {
		TimeManager::SetTimeScale(timeScale);
	}
	if (IsEditFinished()) { TimeManager::Save(); }
}
