#include "EditorApplication.h"
#include "Core/TimeManager.h"
#include "Physics/Physics.h"
#include "Asset/AssetDatabase.h"
#include "Core/JsonFile.h"
#include "Core/Time.h"
#include "Audio/XAudio2/Audio.h"
#include "Diagnostics/Log.h"
#include "Editor/EditorSceneManager.h"
#include "Editor/LogEntries.h"
#include "Editor/Selection.h"
#include "Editor/Undo.h"
#include "Scene/GameObject.h"
#include "Scene/Scene.h"
#include "Scene/SceneManager.h"
#include "Scene/Transform.h"
#include "Script/ScriptEngine.h"
#include "Serialize/SceneSerializer.h"

#include <chrono>
#include <cstdint>
#include <optional>
#include <string>
#include <utility>

using namespace ou;


namespace {

struct SceneSnapshot {
	std::string name;
	std::string path;
	int32_t buildIndex = -1;
	bool isDirty = false;
	Json json;
};

std::optional<SceneSnapshot> &Snapshot() {
	static std::optional<SceneSnapshot> snapshot;
	return snapshot;
}

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

void ReimportPrefabs() {
	for (const auto &guid : AssetDatabase::FindAssets("t:Prefab")) {
		AssetDatabase::ImportAsset(AssetDatabase::GuidToAssetPath(guid));
	}
}

} // namespace



bool ou::EditorApplication::IsPlaying() {
	return sIsPlaying_;
}


void ou::EditorApplication::EnterPlayMode() {
	sRequest_ = Request::Enter;
}


void ou::EditorApplication::ExitPlayMode() {
	sRequest_ = Request::Exit;
}


void ou::EditorApplication::SetPaused(bool isPaused) {
	sIsPaused_ = isPaused;
	if (!isPaused) { sIsStepRequested_ = false; }
}


bool ou::EditorApplication::IsPaused() {
	return sIsPaused_;
}


void ou::EditorApplication::Step() {
	if (!sIsPlaying_) { return; }
	sIsPaused_ = true;
	sIsStepRequested_ = true;
}


bool ou::EditorApplication::IsGameFrameSkipped() {
	return sIsPlaying_ && sSkipsGameFrame_;
}


void ou::EditorApplication::BeginGameFrame(float unscaledDeltaTime) {
	sIsStepFrame_ = false;
	sSkipsGameFrame_ = false;
	if (sIsPlaying_ && sIsPaused_) {
		sIsStepFrame_ = std::exchange(sIsStepRequested_, false);
		sSkipsGameFrame_ = !sIsStepFrame_;
	}

	// 止めている間は時間を進めない。Step は、押すたびに同じだけ進むように fixedDeltaTime だけ進める
	// （FixedUpdate と物理がちょうど 1 回、Update が 1 回動く）
	if (sSkipsGameFrame_) {
		Time::Freeze();
	} else if (sIsStepFrame_) {
		Time::Advance(Time::GetFixedDeltaTime());
	} else {
		Time::Advance(unscaledDeltaTime);
	}

	// Unity と同じく、止めている間は音も止める（Play を止めたら戻す）
	const auto pausesAudio = sIsPlaying_ && sIsPaused_;
	if (pausesAudio != sIsAudioPaused_) {
		sIsAudioPaused_ = pausesAudio;
		Audio::SetEnginePaused(pausesAudio);
	}
}


void ou::EditorApplication::ApplyPendingPlayModeChange() {
	const auto request = std::exchange(sRequest_, Request::None);

	if (request == Request::Enter && !sIsPlaying_) {
		auto *scene = SceneManager::GetActiveScene();
		if (!scene) {
			Log::Warning("シーンがないため Play に入れません");
			return;
		}

		LogEntries::ClearOnPlayIfEnabled();
		// 編集の途中で Play したときも、そこまでを Undo に残す
		Undo::FlushPendingChange(true);

		auto &snapshot = Snapshot();
		snapshot = SceneSnapshot{scene->GetName(), scene->GetPath(), scene->GetBuildIndex(), scene->IsDirty(), SceneSerializer::ToJson(*scene)};

		sIsPlaying_ = true;
		Time::ResetForPlayMode();
		// Project Settings の Time の値から始める
		TimeManager::Apply();
		SceneManager::ReplaceActiveScene(snapshot->name, snapshot->path, snapshot->buildIndex, snapshot->json);
		Log::Info("Play: {}", snapshot->name);
		return;
	}

	if (request == Request::Exit && sIsPlaying_) {
		sIsPlaying_ = false;

		// Play 中にスクリプトで変えた Time・物理の設定を、Project Settings の値に戻す
		// （Project Settings のウィンドウで変えたものは、ファイルに書いてあるので残る）
		TimeManager::Apply();
		Physics::LoadDynamicsManager();

		auto &snapshot = Snapshot();
		if (snapshot) {
			SceneManager::ReplaceActiveScene(snapshot->name, snapshot->path, snapshot->buildIndex, snapshot->json);
			if (snapshot->isDirty) {
				EditorSceneManager::MarkSceneDirty(SceneManager::GetActiveScene());
			}
			Log::Info("Stop: {}", snapshot->name);
		}
		snapshot.reset();
	}
}


void ou::EditorApplication::ReloadScriptsIfChanged() {
	if (sIsPlaying_) { return; }

	// 1 秒ごとにフォルダを見る
	static auto lastCheck = std::chrono::steady_clock::now();
	const auto now = std::chrono::steady_clock::now();
	if (now - lastCheck < std::chrono::seconds(1)) { return; }
	lastCheck = now;

	if (!ScriptEngine::IsOutdated()) { return; }

	AssetDatabase::Refresh();

	auto *scene = SceneManager::GetActiveScene();
	if (!scene) {
		ScriptEngine::Compile();
		ReimportPrefabs();
		return;
	}

	const auto json = SceneSerializer::ToJson(*scene);
	const auto name = scene->GetName();
	const auto path = scene->GetPath();
	const auto buildIndex = scene->GetBuildIndex();
	const auto isDirty = scene->IsDirty();

	const auto *selected = Selection::GetActiveGameObject();
	const auto selectedFileId = selected ? selected->GetFileId() : 0;

	ScriptEngine::Compile();
	ReimportPrefabs();
	SceneManager::ReplaceActiveScene(name, path, buildIndex, json);

	auto *reloaded = SceneManager::GetActiveScene();
	if (isDirty) { EditorSceneManager::MarkSceneDirty(reloaded); }
	// 読み直しは編集ではないので Undo に残さない（書き出す中身が変わることがあるので、記録だけ取り直す）
	Undo::RefreshCurrent(reloaded);
	// アセットを選んでいたときは、そのまま残す
	if (selected) { Selection::SetActiveGameObject(FindGameObjectByFileId(*reloaded, selectedFileId)); }

	Log::Info("スクリプトを読み直しました");
}
