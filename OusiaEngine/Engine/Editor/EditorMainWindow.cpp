#include "EditorMainWindow.h"
#include "Asset/AssetDatabase.h"
#include "Editor/BuildSettingsWindow.h"
#include "Editor/ConsoleWindow.h"
#include "Editor/PreferencesWindow.h"
#include "Editor/EditorApplication.h"
#include "Editor/EditorSceneManager.h"
#include "Editor/GameObjectCommands.h"
#include "Editor/Selection.h"
#include "Editor/GameObjectMenu.h"
#include "Editor/EditorWindow.h"
#include "Editor/GameView.h"
#include "Editor/HierarchyWindow.h"
#include "Editor/InspectorWindow.h"
#include "Editor/ProjectWindow.h"
#include "Editor/SceneView.h"
#include "Editor/Undo.h"
#include "Scene/Scene.h"
#include "Scene/SceneManager.h"

#include <array>
#include <filesystem>
#include <optional>
#include <functional>
#include <string>
#include <utility>

#include <imgui/imgui.h>
#include <imgui/imgui_internal.h>
#include <imgui/imgui_stdlib.h>

using namespace ou;


namespace {

constexpr const char *kSaveChangesPopup = "保存していない変更";
constexpr const char *kSaveAsPopup = "名前を付けて保存";

std::function<void()> sPendingAction;

bool sOpensSaveChangesPopup = false;
bool sOpensSaveAsPopup = false;

std::string sSaveAsPath;

// ウィンドウの × で閉じようとした（Play 中なら Stop してから聞く）
bool sIsQuitRequested = false;
// 保存を聞き終えて、閉じてよくなった
bool sIsQuitApproved = false;


Scene *ActiveScene() {
	return SceneManager::GetActiveScene();
}

void RunPendingAction() {
	if (auto action = std::exchange(sPendingAction, nullptr)) {
		action();
	}
}

void OpenSaveAs() {
	auto *scene = ActiveScene();
	if (!scene) { return; }

	sSaveAsPath = scene->GetPath().empty() ? "Assets/Scenes/" + scene->GetName() + ".scene" : scene->GetPath();
	sOpensSaveAsPopup = true;
}

bool SaveOrAskPath() {
	auto *scene = ActiveScene();
	if (!scene) { return false; }

	if (scene->GetPath().empty()) {
		OpenSaveAs();
		return false;
	}
	return EditorSceneManager::SaveScene(scene);
}

void RunAfterSaveCheck(std::function<void()> action) {
	auto *scene = ActiveScene();
	if (!scene || !scene->IsDirty()) {
		action();
		return;
	}

	sPendingAction = std::move(action);
	sOpensSaveChangesPopup = true;
}

void TogglePause() {
	EditorApplication::SetPaused(!EditorApplication::IsPaused());
}

void TogglePlayMode() {
	if (EditorApplication::IsPlaying()) {
		EditorApplication::ExitPlayMode();
	} else {
		EditorApplication::EnterPlayMode();
		EditorWindow::GetWindow<GameView>();
	}
}

void NewScene() {
	RunAfterSaveCheck([] { EditorSceneManager::NewScene(NewSceneSetup::DefaultGameObjects); });
}

void OpenScene(const std::string &scenePath) {
	RunAfterSaveCheck([scenePath] { EditorSceneManager::OpenScene(scenePath); });
}


void HandleShortcuts() {
	constexpr auto kGlobal = ImGuiInputFlags_RouteGlobal;

	if (ImGui::Shortcut(ImGuiMod_Ctrl | ImGuiKey_P, kGlobal)) { TogglePlayMode(); }
	if (ImGui::Shortcut(ImGuiMod_Ctrl | ImGuiMod_Shift | ImGuiKey_P, kGlobal)) { TogglePause(); }
	if (ImGui::Shortcut(ImGuiMod_Ctrl | ImGuiMod_Alt | ImGuiKey_P, kGlobal | ImGuiInputFlags_Repeat)) { EditorApplication::Step(); }
	if (ImGui::Shortcut(ImGuiMod_Ctrl | ImGuiMod_Shift | ImGuiKey_C, kGlobal)) { EditorWindow::GetWindow<ConsoleWindow>(); }

	if (EditorApplication::IsPlaying()) { return; }

	if (ImGui::Shortcut(ImGuiMod_Ctrl | ImGuiKey_N, kGlobal)) { NewScene(); }
	if (ImGui::Shortcut(ImGuiMod_Ctrl | ImGuiKey_S, kGlobal)) { SaveOrAskPath(); }
	if (ImGui::Shortcut(ImGuiMod_Ctrl | ImGuiMod_Shift | ImGuiKey_S, kGlobal)) { OpenSaveAs(); }

	// 文字を入力している間は、InputText が Ctrl+Z を先に取るので、ここには来ない
	constexpr auto kRepeat = kGlobal | ImGuiInputFlags_Repeat;
	if (ImGui::Shortcut(ImGuiMod_Ctrl | ImGuiKey_Z, kRepeat)) { Undo::PerformUndo(); }
	if (ImGui::Shortcut(ImGuiMod_Ctrl | ImGuiKey_Y, kRepeat) || ImGui::Shortcut(ImGuiMod_Ctrl | ImGuiMod_Shift | ImGuiKey_Z, kRepeat)) {
		Undo::PerformRedo();
	}
}


void DrawMainMenuBar() {
	if (!ImGui::BeginMainMenuBar()) { return; }

	const auto canEdit = !EditorApplication::IsPlaying();

	if (ImGui::BeginMenu("File")) {
		if (ImGui::MenuItem("New Scene", "Ctrl+N", false, canEdit)) { NewScene(); }

		if (ImGui::BeginMenu("Open Scene", canEdit)) {
			const auto guids = AssetDatabase::FindAssets("t:Scene");
			if (guids.empty()) {
				ImGui::TextDisabled("シーンがありません");
			}
			for (const auto &guid : guids) {
				const auto scenePath = AssetDatabase::GuidToAssetPath(guid);
				if (ImGui::MenuItem(scenePath.c_str())) { OpenScene(scenePath); }
			}
			ImGui::EndMenu();
		}

		ImGui::Separator();
		if (ImGui::MenuItem("Save", "Ctrl+S", false, canEdit)) { SaveOrAskPath(); }
		if (ImGui::MenuItem("Save As...", "Ctrl+Shift+S", false, canEdit)) { OpenSaveAs(); }

		ImGui::Separator();
		if (ImGui::MenuItem("Build Settings...")) { EditorWindow::GetWindow<BuildSettingsWindow>(); }
		ImGui::EndMenu();
	}

	if (ImGui::BeginMenu("Edit")) {
		const auto undoLabel = Undo::CanUndo() ? "Undo " + Undo::GetUndoName() : std::string("Undo");
		const auto redoLabel = Undo::CanRedo() ? "Redo " + Undo::GetRedoName() : std::string("Redo");
		if (ImGui::MenuItem(undoLabel.c_str(), "Ctrl+Z", false, canEdit && Undo::CanUndo())) { Undo::PerformUndo(); }
		if (ImGui::MenuItem(redoLabel.c_str(), "Ctrl+Y", false, canEdit && Undo::CanRedo())) { Undo::PerformRedo(); }
		ImGui::Separator();
		GameObjectCommands::DrawMenuItems(Selection::GetActiveGameObject());
		ImGui::Separator();
		if (ImGui::MenuItem("Play", "Ctrl+P", EditorApplication::IsPlaying())) { TogglePlayMode(); }
		if (ImGui::MenuItem("Pause", "Ctrl+Shift+P", EditorApplication::IsPaused())) { TogglePause(); }
		if (ImGui::MenuItem("Step", "Ctrl+Alt+P", false, EditorApplication::IsPlaying())) { EditorApplication::Step(); }
		ImGui::Separator();
		if (ImGui::MenuItem("Preferences...")) { EditorWindow::GetWindow<PreferencesWindow>(); }
		ImGui::EndMenu();
	}

	if (ImGui::BeginMenu("GameObject", canEdit)) {
		GameObjectMenu::DrawItems(ActiveScene(), nullptr);
		ImGui::EndMenu();
	}

	if (ImGui::BeginMenu("Window")) {
		if (ImGui::MenuItem("Hierarchy")) { EditorWindow::GetWindow<HierarchyWindow>(); }
		if (ImGui::MenuItem("Inspector")) { EditorWindow::GetWindow<InspectorWindow>(); }
		if (ImGui::MenuItem("Game")) { EditorWindow::GetWindow<GameView>(); }
		if (ImGui::MenuItem("Scene")) { EditorWindow::GetWindow<SceneView>(); }
		if (ImGui::MenuItem("Project")) { EditorWindow::GetWindow<ProjectWindow>(); }
		if (ImGui::MenuItem("Console", "Ctrl+Shift+C")) { EditorWindow::GetWindow<ConsoleWindow>(); }
		ImGui::EndMenu();
	}

	ImGui::EndMainMenuBar();
}


// ---- ツールバーのアイコンボタン（画像を使わず、図形で描く） ----

constexpr float kIconButtonWidth = 34.0f;

enum class ToolbarIcon {
	Play,
	Pause,
	Step,
};

// Unity と同じく、押されているボタンは青くする
const ImVec4 kButtonOnColor{0.23f, 0.47f, 0.85f, 1.0f};

bool IconButton(const char *id, ToolbarIcon icon, bool isOn, bool isEnabled, const char *tooltip) {
	ImGui::BeginDisabled(!isEnabled);
	const auto isPressed = ImGui::InvisibleButton(id, {kIconButtonWidth, ImGui::GetFrameHeight()});
	ImGui::EndDisabled();
	const auto isHovered = ImGui::IsItemHovered(ImGuiHoveredFlags_AllowWhenDisabled);
	const auto isHeld = ImGui::IsItemActive();

	const auto min = ImGui::GetItemRectMin();
	const auto max = ImGui::GetItemRectMax();
	auto *drawList = ImGui::GetWindowDrawList();

	auto background = ImGui::GetStyleColorVec4(isHeld ? ImGuiCol_ButtonActive : (isHovered && isEnabled) ? ImGuiCol_ButtonHovered : ImGuiCol_Button);
	if (isOn) { background = isHovered ? ImVec4{kButtonOnColor.x * 1.15f, kButtonOnColor.y * 1.15f, kButtonOnColor.z * 1.1f, 1.0f} : kButtonOnColor; }
	drawList->AddRectFilled(min, max, ImGui::GetColorU32(background), ImGui::GetStyle().FrameRounding);

	const auto color = ImGui::GetColorU32(isEnabled ? ImGuiCol_Text : ImGuiCol_TextDisabled);
	const ImVec2 c{(min.x + max.x) * 0.5f, (min.y + max.y) * 0.5f};
	const auto s = (max.y - min.y) * 0.28f; // 図形の半分の高さ

	switch (icon) {
		case ToolbarIcon::Play:
			drawList->AddTriangleFilled({c.x - s * 0.75f, c.y - s}, {c.x - s * 0.75f, c.y + s}, {c.x + s * 0.95f, c.y}, color);
			break;
		case ToolbarIcon::Pause:
			drawList->AddRectFilled({c.x - s * 0.7f, c.y - s}, {c.x - s * 0.22f, c.y + s}, color);
			drawList->AddRectFilled({c.x + s * 0.22f, c.y - s}, {c.x + s * 0.7f, c.y + s}, color);
			break;
		case ToolbarIcon::Step:
			drawList->AddTriangleFilled({c.x - s * 0.9f, c.y - s}, {c.x - s * 0.9f, c.y + s}, {c.x + s * 0.35f, c.y}, color);
			drawList->AddRectFilled({c.x + s * 0.45f, c.y - s}, {c.x + s * 0.85f, c.y + s}, color);
			break;
	}

	if (isHovered) { ImGui::SetTooltip("%s", tooltip); }
	return isPressed && isEnabled;
}


// ---- Play 中の色 ----

// Unity の Playmode tint。Play 中はエディタの地の色を青みがかった色に寄せ、ここで変えた値は Stop で消えることに気づけるようにする
const ImVec4 kPlayModeTint{0.20f, 0.27f, 0.45f, 1.0f};
constexpr float kPlayModeTintAmount = 0.35f;

// 地の色だけを変える（文字は読みやすさのため変えない）
constexpr ImGuiCol kTintedColors[] = {
	ImGuiCol_WindowBg, ImGuiCol_ChildBg, ImGuiCol_PopupBg, ImGuiCol_MenuBarBg,
	ImGuiCol_TitleBg, ImGuiCol_TitleBgActive, ImGuiCol_TitleBgCollapsed,
	ImGuiCol_FrameBg, ImGuiCol_FrameBgHovered, ImGuiCol_FrameBgActive,
	ImGuiCol_Tab, ImGuiCol_TabHovered, ImGuiCol_TabSelected, ImGuiCol_TabDimmed, ImGuiCol_TabDimmedSelected,
	ImGuiCol_DockingEmptyBg, ImGuiCol_TableHeaderBg, ImGuiCol_ScrollbarBg,
};

void ApplyPlayModeTint() {
	static std::optional<std::array<ImVec4, ImGuiCol_COUNT>> baseColors;
	auto &colors = ImGui::GetStyle().Colors;

	const auto isPlaying = EditorApplication::IsPlaying();
	if (isPlaying && !baseColors) {
		baseColors.emplace();
		std::copy(std::begin(colors), std::end(colors), baseColors->begin());
		for (const auto index : kTintedColors) {
			auto &color = colors[index];
			color.x += (kPlayModeTint.x - color.x) * kPlayModeTintAmount;
			color.y += (kPlayModeTint.y - color.y) * kPlayModeTintAmount;
			color.z += (kPlayModeTint.z - color.z) * kPlayModeTintAmount;
		}
	} else if (!isPlaying && baseColors) {
		std::copy(baseColors->begin(), baseColors->end(), std::begin(colors));
		baseColors.reset();
	}
}


void DrawToolbar() {
	auto *viewport = ImGui::GetMainViewport();
	const auto height = ImGui::GetFrameHeight() + ImGui::GetStyle().WindowPadding.y * 2.0f;
	constexpr auto kFlags = ImGuiWindowFlags_NoScrollbar | ImGuiWindowFlags_NoSavedSettings;

	if (ImGui::BeginViewportSideBar("##Toolbar", viewport, ImGuiDir_Up, height, kFlags)) {
		if (const auto *scene = ActiveScene()) {
			ImGui::AlignTextToFramePadding();
			ImGui::Text("%s%s", scene->GetName().c_str(), scene->IsDirty() ? " *" : "");
		}

		// Unity と同じく、真ん中に Play・Pause・Step を並べる
		constexpr float kSpacing = 1.0f;
		const auto totalWidth = kIconButtonWidth * 3.0f + kSpacing * 2.0f;
		ImGui::SameLine((ImGui::GetWindowWidth() - totalWidth) * 0.5f);
		ImGui::PushStyleVar(ImGuiStyleVar_ItemSpacing, {kSpacing, ImGui::GetStyle().ItemSpacing.y});

		const auto isPlaying = EditorApplication::IsPlaying();
		if (IconButton("##Play", ToolbarIcon::Play, isPlaying, true, isPlaying ? "Stop (Ctrl+P)" : "Play (Ctrl+P)")) { TogglePlayMode(); }
		ImGui::SameLine();
		if (IconButton("##Pause", ToolbarIcon::Pause, EditorApplication::IsPaused(), true, "Pause (Ctrl+Shift+P)")) { TogglePause(); }
		ImGui::SameLine();
		if (IconButton("##Step", ToolbarIcon::Step, false, isPlaying, "Step (Ctrl+Alt+P)")) { EditorApplication::Step(); }

		ImGui::PopStyleVar();
	}
	ImGui::End();
}


void DrawSaveChangesPopup() {
	if (std::exchange(sOpensSaveChangesPopup, false)) {
		ImGui::OpenPopup(kSaveChangesPopup);
	}
	if (!ImGui::BeginPopupModal(kSaveChangesPopup, nullptr, ImGuiWindowFlags_AlwaysAutoResize)) { return; }

	const auto *scene = ActiveScene();
	ImGui::Text("シーン \"%s\" に保存していない変更があります。", scene ? scene->GetName().c_str() : "");

	if (ImGui::Button("保存")) {
		ImGui::CloseCurrentPopup();
		if (SaveOrAskPath()) { RunPendingAction(); }
	}
	ImGui::SameLine();
	if (ImGui::Button("保存しない")) {
		ImGui::CloseCurrentPopup();
		RunPendingAction();
	}
	ImGui::SameLine();
	if (ImGui::Button("キャンセル")) {
		ImGui::CloseCurrentPopup();
		sPendingAction = nullptr;
	}

	ImGui::EndPopup();
}


void DrawSaveAsPopup() {
	if (std::exchange(sOpensSaveAsPopup, false)) {
		ImGui::OpenPopup(kSaveAsPopup);
	}
	if (!ImGui::BeginPopupModal(kSaveAsPopup, nullptr, ImGuiWindowFlags_AlwaysAutoResize)) { return; }

	ImGui::TextUnformatted("保存先（Assets/ から始まるパス）");
	if (ImGui::IsWindowAppearing()) {
		ImGui::SetKeyboardFocusHere();
	}
	ImGui::SetNextItemWidth(480.0f);
	const auto isEntered = ImGui::InputText("##Path", &sSaveAsPath, ImGuiInputTextFlags_EnterReturnsTrue);

	const auto isValid = sSaveAsPath.starts_with("Assets/") && sSaveAsPath.ends_with(".scene");
	if (!isValid) {
		ImGui::TextColored({1.0f, 0.4f, 0.4f, 1.0f}, "Assets/ で始まり、.scene で終わるパスにしてください");
	}

	ImGui::BeginDisabled(!isValid);
	const auto isSaveClicked = ImGui::Button("保存");
	ImGui::EndDisabled();

	if ((isSaveClicked || isEntered) && isValid) {
		if (EditorSceneManager::SaveScene(ActiveScene(), sSaveAsPath)) {
			ImGui::CloseCurrentPopup();
			RunPendingAction();
		}
	}
	ImGui::SameLine();
	if (ImGui::Button("キャンセル")) {
		ImGui::CloseCurrentPopup();
		sPendingAction = nullptr;
	}

	ImGui::EndPopup();
}

// 初回起動時のみウィンドウの配置を決める
void BuildDefaultLayout(ImGuiID dockspaceId) {
	auto *node = ImGui::DockBuilderGetNode(dockspaceId);
	if (node && node->IsSplitNode()) { return; }

	ImGui::DockBuilderSetNodeSize(dockspaceId, ImGui::GetMainViewport()->WorkSize);
	ImGuiID left = 0;
	ImGuiID rest = 0;
	ImGuiID right = 0;
	ImGuiID center = 0;
	ImGuiID bottom = 0;
	ImGui::DockBuilderSplitNode(dockspaceId, ImGuiDir_Left, 0.2f, &left, &rest);
	ImGui::DockBuilderSplitNode(rest, ImGuiDir_Right, 0.3f, &right, &center);
	ImGui::DockBuilderSplitNode(center, ImGuiDir_Down, 0.3f, &bottom, &center);
	ImGui::DockBuilderDockWindow("Hierarchy", left);
	ImGui::DockBuilderDockWindow("Inspector", right);
	ImGui::DockBuilderDockWindow("Scene", center);
	ImGui::DockBuilderDockWindow("Game", center);
	ImGui::DockBuilderDockWindow("Project", bottom);
	ImGui::DockBuilderDockWindow("Console", bottom);
}

} // namespace



void ou::EditorMainWindow::Draw() {
	static bool isFirstDraw = true;
	if (std::exchange(isFirstDraw, false)) {
		EditorWindow::GetWindow<HierarchyWindow>();
		EditorWindow::GetWindow<InspectorWindow>();
		EditorWindow::GetWindow<GameView>();
		EditorWindow::GetWindow<SceneView>();
		EditorWindow::GetWindow<ProjectWindow>();
		EditorWindow::GetWindow<ConsoleWindow>();
	}

	ApplyPlayModeTint();
	HandleShortcuts();

	DrawMainMenuBar();
	DrawToolbar();
	const auto dockspaceId = ImGui::DockSpaceOverViewport();

	static bool isLayoutChecked = false;
	if (std::exchange(isLayoutChecked, true) == false) {
		BuildDefaultLayout(dockspaceId);
	}

	EditorWindow::DrawAll();

	// Unity と同じく、Play 中の変更は保存しないので、Stop してから Edit のシーンについて聞く
	if (sIsQuitRequested && !EditorApplication::IsPlaying()) {
		sIsQuitRequested = false;
		RunAfterSaveCheck([] { sIsQuitApproved = true; });
	}

	DrawSaveChangesPopup();
	DrawSaveAsPopup();

	// このフレームの変更を、入力が終わっていれば 1 つの操作として Undo に残す
	Undo::FlushPendingChange(false);
}


void ou::EditorMainWindow::RequestOpenScene(const std::string &scenePath) {
	if (EditorApplication::IsPlaying()) { return; }
	OpenScene(scenePath);
}


void ou::EditorMainWindow::RequestQuit() {
	sIsQuitRequested = true;
	if (EditorApplication::IsPlaying()) {
		EditorApplication::ExitPlayMode();
	}
}


bool ou::EditorMainWindow::IsQuitApproved() {
	return sIsQuitApproved;
}
