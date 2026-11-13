#include "EditorApplication.h"
#include "Core/Time.h"
#include "Diagnostics/Log.h"
#include "Editor/EditorSceneManager.h"
#include "Scene/Scene.h"
#include "Scene/SceneManager.h"
#include "Serialize/SceneSerializer.h"

#include <cstdint>
#include <optional>
#include <string>
#include <utility>

#include <nlohmann/json.hpp>

using namespace ou;


namespace {

struct SceneSnapshot {
	std::string name;
	std::string path;
	int32_t buildIndex = -1;
	bool isDirty = false;
	nlohmann::ordered_json json;
};

std::optional<SceneSnapshot> &Snapshot() {
	static std::optional<SceneSnapshot> snapshot;
	return snapshot;
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


void ou::EditorApplication::ApplyPendingPlayModeChange() {
	const auto request = std::exchange(sRequest_, Request::None);

	if (request == Request::Enter && !sIsPlaying_) {
		auto *scene = SceneManager::GetActiveScene();
		if (!scene) {
			Log::Warning("シーンがないため Play に入れません");
			return;
		}

		auto &snapshot = Snapshot();
		snapshot = SceneSnapshot{scene->GetName(), scene->GetPath(), scene->GetBuildIndex(), scene->IsDirty(), SceneSerializer::ToJson(*scene)};

		sIsPlaying_ = true;
		Time::ResetForPlayMode();
		SceneManager::ReplaceActiveScene(snapshot->name, snapshot->path, snapshot->buildIndex, snapshot->json);
		Log::Info("Play: {}", snapshot->name);
		return;
	}

	if (request == Request::Exit && sIsPlaying_) {
		sIsPlaying_ = false;

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
