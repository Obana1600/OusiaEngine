#include "EditorMainWindow.h"
#include "Asset/AssetDatabase.h"
#include "Editor/BuildSettingsWindow.h"
#include "Editor/ConsoleWindow.h"
#include "Editor/PreferencesWindow.h"
#include "Editor/EditorApplication.h"
#include "Editor/EditorSceneManager.h"
#include "Editor/GameObjectMenu.h"
#include "Editor/EditorWindow.h"
#include "Editor/GameView.h"
#include "Editor/HierarchyWindow.h"
#include "Editor/InspectorWindow.h"
#include "Editor/ProjectWindow.h"
#include "Editor/SceneView.h"
#include "Scene/Scene.h"
#include "Scene/SceneManager.h"

#include <filesystem>
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
	if (ImGui::Shortcut(ImGuiMod_Ctrl | ImGuiMod_Shift | ImGuiKey_C, kGlobal)) { EditorWindow::GetWindow<ConsoleWindow>(); }

	if (EditorApplication::IsPlaying()) { return; }

	if (ImGui::Shortcut(ImGuiMod_Ctrl | ImGuiKey_N, kGlobal)) { NewScene(); }
	if (ImGui::Shortcut(ImGuiMod_Ctrl | ImGuiKey_S, kGlobal)) { SaveOrAskPath(); }
	if (ImGui::Shortcut(ImGuiMod_Ctrl | ImGuiMod_Shift | ImGuiKey_S, kGlobal)) { OpenSaveAs(); }
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


void DrawToolbar() {
	auto *viewport = ImGui::GetMainViewport();
	const auto height = ImGui::GetFrameHeight() + ImGui::GetStyle().WindowPadding.y * 2.0f;
	constexpr auto kFlags = ImGuiWindowFlags_NoScrollbar | ImGuiWindowFlags_NoSavedSettings;

	if (ImGui::BeginViewportSideBar("##Toolbar", viewport, ImGuiDir_Up, height, kFlags)) {
		if (const auto *scene = ActiveScene()) {
			ImGui::AlignTextToFramePadding();
			ImGui::Text("%s%s", scene->GetName().c_str(), scene->IsDirty() ? " *" : "");
		}

		constexpr float kButtonWidth = 80.0f;
		ImGui::SameLine((ImGui::GetWindowWidth() - kButtonWidth) * 0.5f);
		if (ImGui::Button(EditorApplication::IsPlaying() ? "Stop" : "Play", {kButtonWidth, 0.0f})) {
			TogglePlayMode();
		}
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
