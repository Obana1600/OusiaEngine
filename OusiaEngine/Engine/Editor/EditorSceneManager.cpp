#include "EditorSceneManager.h"
#include "Asset/AssetDatabase.h"
#include "Core/StringUtil.h"
#include "Diagnostics/Log.h"
#include "Editor/EditorApplication.h"
#include "Math/Mathf.h"
#include "Render/Camera.h"
#include "Render/Light.h"
#include "Scene/GameObject.h"
#include "Scene/Scene.h"
#include "Scene/SceneManager.h"
#include "Scene/Transform.h"
#include "Serialize/SceneSerializer.h"

#include <filesystem>
#include <fstream>
#include <string>
#include <system_error>

#include <nlohmann/json.hpp>

using namespace ou;


namespace {

std::string MakeSceneName(std::string_view scenePath) {
	return WideToUtf8(std::filesystem::path(Utf8ToWide(scenePath)).stem().wstring());
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
		light->GetTransform()->SetLocalEulerAngles(Vector3{50.0f, -30.0f, 0.0f} * Mathf::Deg2Rad);
	}

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

	std::ifstream file(*fullPath, std::ios::binary);
	if (!file) {
		Log::Error("シーンのファイルを開けませんでした: {}", scenePath);
		return nullptr;
	}

	const auto json = nlohmann::ordered_json::parse(file, nullptr, false, true);
	if (json.is_discarded()) {
		Log::Error("シーンの json の書き方が間違っています: {}", scenePath);
		return nullptr;
	}

	if (!SceneSerializer::ValidateRoot(json)) {
		Log::Error("シーンのファイルではありません: {}", scenePath);
		return nullptr;
	}

	SceneManager::ReplaceActiveScene(MakeSceneName(scenePath), scenePath, SceneManager::FindBuildIndexByPath(scenePath), json);
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

	// 新しいファイルに .meta と GUID を付ける
	if (isNewFile) {
		AssetDatabase::Refresh();
	}

	Log::Info("シーンを保存しました: {}", scenePath);
	return true;
}


bool ou::EditorSceneManager::MarkSceneDirty(Scene *scene) {
	if (!scene || EditorApplication::IsPlaying()) { return false; }

	scene->isDirty_ = true;
	return true;
}
