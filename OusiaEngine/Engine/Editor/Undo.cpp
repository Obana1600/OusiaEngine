#include "Undo.h"
#include "Diagnostics/Log.h"
#include "Editor/EditorApplication.h"
#include "Editor/EditorSceneManager.h"
#include "Editor/Selection.h"
#include "Scene/GameObject.h"
#include "Scene/Scene.h"
#include "Scene/SceneManager.h"
#include "Scene/Transform.h"
#include "Serialize/SceneSerializer.h"

#include <cstdint>
#include <deque>
#include <string>
#include <utility>
#include <vector>

#include <imgui/imgui.h>
#include <ImGuizmo/ImGuizmo.h> // imgui/imgui.h の後ろに置く

using namespace ou;


namespace {

constexpr const char *kDefaultName = "Change";

// 残しておく操作の数（シーンを丸ごと持つので、多くしすぎない）
constexpr size_t kMaxSteps = 100;

// ある時点のシーン。id が同じなら同じ中身
struct State {
	Json json;
	uint64_t id = 0;
};

struct Step {
	std::string name;
	State state; // Undo の履歴なら戻した先、Redo の履歴ならやり直した先
};

enum class Request : uint8_t {
	None,
	Undo,
	Redo,
};

std::deque<Step> sUndoSteps;
std::vector<Step> sRedoSteps;

// 最後に記録したときのシーン。変わったかは、これと比べる
State sCurrent;
bool sHasCurrent = false;
uint64_t sNextId = 1;
// 保存したときの State の id（戻して保存した状態に戻ったら、* を消す）
uint64_t sSavedId = 0;

// MarkSceneDirty が呼ばれて、まだ記録していない
bool sHasPendingChange = false;
std::string sPendingName;

Request sRequest = Request::None;

GameObject *FindGameObjectByFileId(GameObject *gameObject, uint64_t fileId) {
	if (gameObject->GetFileId() == fileId) { return gameObject; }
	for (auto *child : gameObject->GetTransform()->GetChildren()) {
		if (auto *found = FindGameObjectByFileId(child->GetGameObject(), fileId)) { return found; }
	}
	return nullptr;
}

GameObject *FindGameObjectByFileId(Scene &scene, uint64_t fileId) {
	if (fileId == 0) { return nullptr; }
	for (auto *root : scene.GetRootGameObjects()) {
		if (auto *found = FindGameObjectByFileId(root, fileId)) { return found; }
	}
	return nullptr;
}

// ドラッグや文字の入力の途中か（途中なら、終わるまで 1 つの操作にまとめる）
bool IsEditing() {
	return ImGui::IsAnyItemActive() || ImGuizmo::IsUsingAny() || ImGui::IsMouseDown(ImGuiMouseButton_Left);
}

} // namespace



void ou::Undo::SetCurrentGroupName(std::string_view name) {
	sPendingName = name;
}


void ou::Undo::PerformUndo() {
	if (EditorApplication::IsPlaying()) { return; }
	sRequest = Request::Undo;
}


void ou::Undo::PerformRedo() {
	if (EditorApplication::IsPlaying()) { return; }
	sRequest = Request::Redo;
}


bool ou::Undo::CanUndo() {
	return !sUndoSteps.empty() || sHasPendingChange;
}


bool ou::Undo::CanRedo() {
	return !sRedoSteps.empty() && !sHasPendingChange;
}


std::string ou::Undo::GetUndoName() {
	if (sHasPendingChange) { return sPendingName.empty() ? kDefaultName : sPendingName; }
	return sUndoSteps.empty() ? std::string() : sUndoSteps.back().name;
}


std::string ou::Undo::GetRedoName() {
	return CanRedo() ? sRedoSteps.back().name : std::string();
}


void ou::Undo::ClearAll() {
	sUndoSteps.clear();
	sRedoSteps.clear();
	sHasPendingChange = false;
	sPendingName.clear();
	sRequest = Request::None;
}


void ou::Undo::ResetHistory(Scene *scene, bool isSaved) {
	ClearAll();
	sHasCurrent = false;
	if (!scene) { return; }

	sCurrent = State{SceneSerializer::ToJson(*scene), sNextId++};
	sHasCurrent = true;
	sSavedId = isSaved ? sCurrent.id : 0;
}


void ou::Undo::RefreshCurrent(Scene *scene) {
	sHasPendingChange = false;
	sPendingName.clear();
	if (!scene || !sHasCurrent) { return; }

	// 書き出す中身が変わっただけなので id はそのまま（保存した状態かどうかは変えない）
	sCurrent.json = SceneSerializer::ToJson(*scene);
}


void ou::Undo::NotifySceneChanged(Scene *scene) {
	if (!sHasCurrent || EditorApplication::IsPlaying()) { return; }
	if (!scene || scene != SceneManager::GetActiveScene()) { return; }

	sHasPendingChange = true;
}


void ou::Undo::NotifySceneSaved(Scene *scene) {
	if (!scene || scene != SceneManager::GetActiveScene() || !sHasCurrent) { return; }

	// 保存した中身を、記録してある最後の状態にそろえる
	FlushPendingChange(true);
	sSavedId = sCurrent.id;
}


void ou::Undo::FlushPendingChange(bool force) {
	// Play 中など、名前だけ付いて記録しなかったものは捨てる
	if (!sHasPendingChange) {
		sPendingName.clear();
		return;
	}
	if (!force && IsEditing()) { return; }

	sHasPendingChange = false;
	auto name = std::exchange(sPendingName, std::string());

	auto *scene = SceneManager::GetActiveScene();
	if (!scene || !sHasCurrent || EditorApplication::IsPlaying()) { return; }

	auto json = SceneSerializer::ToJson(*scene);
	// 値を動かして元に戻したなど、中身が同じなら残さない
	if (json == sCurrent.json) { return; }

	sUndoSteps.push_back(Step{name.empty() ? kDefaultName : std::move(name), std::move(sCurrent)});
	while (sUndoSteps.size() > kMaxSteps) {
		sUndoSteps.pop_front();
	}
	sRedoSteps.clear();

	sCurrent = State{std::move(json), sNextId++};
}


void ou::Undo::ApplyPendingRequest() {
	const auto request = std::exchange(sRequest, Request::None);
	if (request == Request::None || EditorApplication::IsPlaying() || !sHasCurrent) { return; }

	// 入力の途中で押されたときは、そこまでを 1 つの操作として残してから戻す
	FlushPendingChange(true);

	if (request == Request::Undo) {
		if (sUndoSteps.empty()) { return; }

		auto step = std::move(sUndoSteps.back());
		sUndoSteps.pop_back();
		sRedoSteps.push_back(Step{step.name, std::move(sCurrent)});
		sCurrent = std::move(step.state);
		Restore(sCurrent.json, sCurrent.id != sSavedId);
		Log::Info("Undo: {}", step.name);
		return;
	}

	if (sRedoSteps.empty()) { return; }

	auto step = std::move(sRedoSteps.back());
	sRedoSteps.pop_back();
	sUndoSteps.push_back(Step{step.name, std::move(sCurrent)});
	sCurrent = std::move(step.state);
	Restore(sCurrent.json, sCurrent.id != sSavedId);
	Log::Info("Redo: {}", step.name);
}


void ou::Undo::Restore(const Json &json, bool isDirty) {
	const auto *scene = SceneManager::GetActiveScene();
	if (!scene) { return; }

	const auto name = scene->GetName();
	const auto path = scene->GetPath();
	const auto buildIndex = scene->GetBuildIndex();

	// 読み直すと別のオブジェクトになるので、選んでいたものは fileId で引き直す
	const auto *selected = Selection::GetActiveGameObject();
	const auto selectedFileId = selected ? selected->GetFileId() : 0;

	SceneManager::ReplaceActiveScene(name, path, buildIndex, json);

	auto *restored = SceneManager::GetActiveScene();
	EditorSceneManager::SetDirtyFlag(restored, isDirty);
	// アセットを選んでいたときは、そのまま残す
	if (selected) { Selection::SetActiveGameObject(FindGameObjectByFileId(*restored, selectedFileId)); }
}
