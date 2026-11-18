#include "EditorSceneManager.h"
#include "Asset/AssetDatabase.h"
#include "Core/JsonFile.h"
#include "Core/StringUtil.h"
#include "Diagnostics/Log.h"
#include "Editor/EditorApplication.h"
#include "Editor/EditorBuildSettings.h"
#include "Editor/Undo.h"
#include "Math/Mathf.h"
#include "Platform/Paths.h"
#include "Render/Camera.h"
#include "Render/Light.h"
#include "Scene/GameObject.h"
#include "Scene/Scene.h"
#include "Scene/SceneManager.h"
#include "Scene/Transform.h"
#include "Serialize/SceneSerializer.h"

#include <algorithm>
#include <filesystem>
#include <optional>
#include <string>
#include <system_error>

using namespace ou;


namespace {

std::string MakeSceneName(std::string_view scenePath) {
	return WideToUtf8(std::filesystem::path(Utf8ToWide(scenePath)).stem().wstring());
}

std::optional<std::filesystem::path> FindLastScenePath() {
	const auto folder = FindUserSettingsFolder();
	if (!folder) { return std::nullopt; }
	return *folder / L"LastScene.json";
}

} // namespace



Scene *ou::EditorSceneManager::NewScene(NewSceneSetup setup) {
	if (EditorApplication::IsPlaying()) {
		Log::Error("Play 中は新しいシーンを作れません");
		return nullptr;
	}

	Scene empty("Untitled", -1);
	SceneManager::ReplaceActiveScene("Untitled", {}, -1, SceneSerializer::ToJson(empty));
	auto *scene = SceneManager::GetActiveScene();

	if (setup == NewSceneSetup::DefaultGameObjects) {
		auto *camera = scene->CreateGameObject("Main Camera");
		camera->AddComponent<Camera>();
		camera->GetTransform()->SetLocalPosition({0.0f, 1.0f, -10.0f});

		auto *light = scene->CreateGameObject("Directional Light");
		light->AddComponent<Light>()->SetType(LightType::Directional);
		light->GetTransform()->SetLocalPosition({0.0f, 3.0f, 0.0f});
		light->GetTransform()->SetLocalEulerAngles(Vector3(50.0f, -30.0f, 0.0f) * Mathf::Deg2Rad);
	}

	Undo::ResetHistory(scene, true);
	return scene;
}


Scene *ou::EditorSceneManager::OpenScene(std::string_view scenePath) {
	if (EditorApplication::IsPlaying()) {
		Log::Error("Play 中はシーンを開けません: {}", scenePath);
		return nullptr;
	}

	const auto fullPath = AssetDatabase::AssetPathToFullPath(scenePath);
	if (!fullPath) {
		Log::Error("シーンのパスは Assets/ か EngineAssets/ で始めてください: {}", scenePath);
		return nullptr;
	}

	// 開けない・書き方が間違っているときは JsonFile がパス付きでログに出す
	const auto json = JsonFile::Load(*fullPath);
	if (!json) { return nullptr; }

	if (!SceneSerializer::ValidateRoot(*json)) {
		Log::Error("シーンのファイルではありません: {}", scenePath);
		return nullptr;
	}

	SceneManager::ReplaceActiveScene(MakeSceneName(scenePath), scenePath, SceneManager::FindBuildIndexByPath(scenePath), *json);
	RememberLastScene(scenePath);
	Undo::ResetHistory(SceneManager::GetActiveScene(), true);
	return SceneManager::GetActiveScene();
}


bool ou::EditorSceneManager::SaveScene(Scene *scene, std::string_view dstScenePath) {
	if (!scene) { return false; }

	if (EditorApplication::IsPlaying()) {
		Log::Error("Play 中はシーンを保存できません: {}", scene->GetName());
		return false;
	}

	const auto scenePath = dstScenePath.empty() ? scene->GetPath() : std::string(dstScenePath);
	if (scenePath.empty()) {
		Log::Error("保存先のパスがありません: {}", scene->GetName());
		return false;
	}

	const auto fullPath = AssetDatabase::AssetPathToFullPath(scenePath);
	if (!fullPath) {
		Log::Error("シーンのパスは Assets/ か EngineAssets/ で始めてください: {}", scenePath);
		return false;
	}

	std::error_code ec;
	const auto isNewFile = !std::filesystem::exists(*fullPath, ec);
	std::filesystem::create_directories(fullPath->parent_path(), ec);

	if (!SceneSerializer::Save(*scene, *fullPath)) { return false; }

	scene->path_ = scenePath;
	scene->name_ = MakeSceneName(scenePath);
	scene->buildIndex_ = SceneManager::FindBuildIndexByPath(scenePath);
	scene->isDirty_ = false;
	Undo::NotifySceneSaved(scene);

	// 新しいファイルに .meta と GUID を付ける
	if (isNewFile) {
		AssetDatabase::Refresh();
	}

	RememberLastScene(scenePath);
	Log::Info("シーンを保存しました: {}", scenePath);
	return true;
}


bool ou::EditorSceneManager::MarkSceneDirty(Scene *scene) {
	if (!scene || EditorApplication::IsPlaying()) { return false; }

	scene->isDirty_ = true;
	Undo::NotifySceneChanged(scene);
	return true;
}


void ou::EditorSceneManager::NotifyAssetMoved(const std::string &oldPath, const std::string &newPath) {
	const auto isSameOrInside = [&](const std::string &path) { return path == oldPath || path.starts_with(oldPath + "/"); };

	// 開いているシーン（直さないと、保存したときに古いパスへ書き戻す）
	if (auto *scene = SceneManager::GetActiveScene(); scene && !scene->path_.empty() && isSameOrInside(scene->path_)) {
		scene->path_ = newPath + scene->path_.substr(oldPath.size());
		scene->name_ = MakeSceneName(scene->path_);
		RememberLastScene(scene->path_);
	} else if (const auto lastPath = LoadLastScenePath(); !lastPath.empty() && isSameOrInside(lastPath)) {
		RememberLastScene(newPath + lastPath.substr(oldPath.size()));
	}

	// Build Settings は GUID から今のパスを引き直して書き直す（シーンを含まないときは何も変わらない）
	auto scenes = EditorBuildSettings::GetScenes();
	const auto hasMovedScene = std::any_of(scenes.begin(), scenes.end(), [&](const EditorBuildSettingsScene &scene) {
		return isSameOrInside(scene.path) || scene.path.starts_with(newPath);
	});
	if (hasMovedScene) { EditorBuildSettings::SetScenes(scenes); }
}


void ou::EditorSceneManager::SetDirtyFlag(Scene *scene, bool isDirty) {
	if (scene) { scene->isDirty_ = isDirty; }
}


void ou::EditorSceneManager::OpenLastScene() {
	if (const auto lastPath = LoadLastScenePath(); !lastPath.empty() && OpenScene(lastPath)) { return; }
	if (!SceneManager::sBuildScenes_.empty() && OpenScene(SceneManager::sBuildScenes_.front().path)) { return; }

	NewScene(NewSceneSetup::DefaultGameObjects);
}


void ou::EditorSceneManager::RememberLastScene(std::string_view scenePath) {
	const auto path = FindLastScenePath();
	if (!path) { return; }

	auto json = Json::object();
	json["path"] = std::string(scenePath);
	JsonFile::Save(*path, json);
}


std::string ou::EditorSceneManager::LoadLastScenePath() {
	const auto path = FindLastScenePath();
	if (!path) { return {}; }

	std::error_code error;
	if (!std::filesystem::exists(*path, error)) { return {}; }

	const auto json = JsonFile::Load(*path);
	if (!json || !json->is_object()) { return {}; }
	return json->value("path", std::string{});
}
