#include "Undo.h"
#include "Diagnostics/Log.h"
#include "Editor/EditorApplication.h"
#include "Editor/EditorSceneManager.h"
#include "Scene/Scene.h"
#include "Scene/SceneManager.h"
#include "Serialize/SceneSerializer.h"

#include <cstdint>
#include <deque>
#include <optional>
#include <string>
#include <unordered_map>
#include <utility>
#include <vector>

#include <imgui/imgui.h>
#include <ImGuizmo/ImGuizmo.h> // imgui/imgui.h の後ろに置く

using namespace ou;


namespace {

constexpr const char *kDefaultName = "Change";

// 残しておく操作の数
constexpr size_t kMaxSteps = 100;

// GameObject 1 個の、ある時点の中身
struct GameObjectState {
	Json json; // ToJson(scene) の gameObjects の要素（Component も入っている）
	int siblingIndex = 0;

	bool operator==(const GameObjectState &) const = default;
};

// シーンの、ある時点の中身。キーは GameObject の fileId
using SceneState = std::unordered_map<uint64_t, GameObjectState>;

struct GameObjectChange {
	uint64_t id = 0;
	std::optional<GameObjectState> before; // なければ、その操作で作った
	std::optional<GameObjectState> after;  // なければ、その操作で消した
};

struct Step {
	std::string name;
	std::vector<GameObjectChange> changes;
	// 操作の前と後の状態の番号（番号が同じなら同じ中身。保存した状態に戻ったかを知るのに使う）
	uint64_t beforeId = 0;
	uint64_t afterId = 0;
};

enum class Request : uint8_t {
	None,
	Undo,
	Redo,
};

std::deque<Step> sUndoSteps;
std::vector<Step> sRedoSteps;

// 最後に記録したときのシーン。変わったかは、これと比べる
SceneState sCurrent;
uint64_t sCurrentId = 0;
bool sHasCurrent = false;
uint64_t sNextId = 1;
// 保存したときの状態の番号（戻して保存した状態に戻ったら、* を消す）
uint64_t sSavedId = 0;

// MarkSceneDirty が呼ばれて、まだ記録していない
bool sHasPendingChange = false;
std::string sPendingName;

Request sRequest = Request::None;

uint64_t ReadId(const Json &json) {
	const auto it = json.find("$id");
	return it != json.end() && it->is_number_unsigned() ? it->get<uint64_t>() : 0;
}

uint64_t ReadParentTransformId(const Json &gameObjectJson) {
	const auto components = gameObjectJson.find("components");
	if (components == gameObjectJson.end() || !components->is_array() || components->empty()) { return 0; }

	const auto &transform = components->front();
	const auto parent = transform.find("parent");
	if (parent == transform.end() || !parent->is_object()) { return 0; }
	const auto id = parent->find("id");
	return id != parent->end() && id->is_number_unsigned() ? id->get<uint64_t>() : 0;
}

SceneState ToSceneState(Scene &scene) {
	auto json = SceneSerializer::ToJson(scene);

	// gameObjects は兄弟を今の並びで入れているので、親ごとに数えれば兄弟の中での位置になる
	SceneState state;
	std::unordered_map<uint64_t, int> childCounts;
	for (auto &gameObject : json["gameObjects"]) {
		const auto id = ReadId(gameObject);
		const auto siblingIndex = childCounts[ReadParentTransformId(gameObject)]++;
		state.insert_or_assign(id, GameObjectState{std::move(gameObject), siblingIndex});
	}
	return state;
}

std::vector<GameObjectChange> CompareSceneStates(const SceneState &before, const SceneState &after) {
	std::vector<GameObjectChange> changes;
	for (const auto &[id, state] : before) {
		const auto it = after.find(id);
		if (it == after.end()) {
			changes.push_back({id, state, std::nullopt});
		} else if (!(it->second == state)) {
			changes.push_back({id, state, it->second});
		}
	}
	for (const auto &[id, state] : after) {
		if (!before.contains(id)) { changes.push_back({id, std::nullopt, state}); }
	}
	return changes;
}

// changes の、isUndo なら前・そうでなければ後の中身にシーンを戻し、sCurrent もそれにそろえる
void ApplyChanges(Scene &scene, const std::vector<GameObjectChange> &changes, bool isUndo) {
	std::vector<SceneSerializer::RestoringGameObject> targets;
	targets.reserve(changes.size());
	for (const auto &change : changes) {
		const auto &state = isUndo ? change.before : change.after;
		targets.push_back({change.id, state ? &state->json : nullptr, state ? state->siblingIndex : 0});

		if (state) {
			sCurrent.insert_or_assign(change.id, *state);
		} else {
			sCurrent.erase(change.id);
		}
	}
	SceneSerializer::RestoreGameObjects(scene, targets);

	// 戻したあとのシーンが記録と違えば、どこかの Component が読み直しに対応できていない
	auto restored = ToSceneState(scene);
	if (restored != sCurrent) {
		for (const auto &change : CompareSceneStates(sCurrent, restored)) {
			const auto &json = change.after ? change.after->json : change.before->json;
			Log::Warning("Undo で戻した結果が記録と違います: {}", json.value("name", std::string()));
		}
		sCurrent = std::move(restored);
	}
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
	sCurrent.clear();
	if (!scene) { return; }

	sCurrent = ToSceneState(*scene);
	sCurrentId = sNextId++;
	sHasCurrent = true;
	sSavedId = isSaved ? sCurrentId : 0;
}


void ou::Undo::RefreshCurrent(Scene *scene) {
	sHasPendingChange = false;
	sPendingName.clear();
	if (!scene || !sHasCurrent) { return; }

	// 書き出す中身が変わっただけなので番号はそのまま（保存した状態かどうかは変えない）
	sCurrent = ToSceneState(*scene);
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
	sSavedId = sCurrentId;
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

	auto state = ToSceneState(*scene);
	auto changes = CompareSceneStates(sCurrent, state);
	// 値を動かして元に戻したなど、中身が同じなら残さない
	if (changes.empty()) { return; }

	const auto afterId = sNextId++;
	sUndoSteps.push_back(Step{name.empty() ? kDefaultName : std::move(name), std::move(changes), sCurrentId, afterId});
	while (sUndoSteps.size() > kMaxSteps) {
		sUndoSteps.pop_front();
	}
	sRedoSteps.clear();

	sCurrent = std::move(state);
	sCurrentId = afterId;
}


void ou::Undo::ApplyPendingRequest() {
	const auto request = std::exchange(sRequest, Request::None);
	if (request == Request::None || EditorApplication::IsPlaying() || !sHasCurrent) { return; }

	auto *scene = SceneManager::GetActiveScene();
	if (!scene) { return; }

	// 入力の途中で押されたときは、そこまでを 1 つの操作として残してから戻す
	FlushPendingChange(true);

	const auto isUndo = request == Request::Undo;
	if (isUndo ? sUndoSteps.empty() : sRedoSteps.empty()) { return; }

	Step step;
	if (isUndo) {
		step = std::move(sUndoSteps.back());
		sUndoSteps.pop_back();
	} else {
		step = std::move(sRedoSteps.back());
		sRedoSteps.pop_back();
	}

	ApplyChanges(*scene, step.changes, isUndo);
	sCurrentId = isUndo ? step.beforeId : step.afterId;
	EditorSceneManager::SetDirtyFlag(scene, sCurrentId != sSavedId);
	Log::Info("{}: {}", isUndo ? "Undo" : "Redo", step.name);

	if (isUndo) {
		sRedoSteps.push_back(std::move(step));
	} else {
		sUndoSteps.push_back(std::move(step));
	}
}
