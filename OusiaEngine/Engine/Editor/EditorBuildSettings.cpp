#include "EditorBuildSettings.h"
#include "Core/JsonFile.h"
#include "Asset/AssetDatabase.h"
#include "Core/StringUtil.h"
#include "Diagnostics/Log.h"
#include "Platform/Paths.h"
#include "Scene/SceneManager.h"

#include <filesystem>
#include <optional>
#include <utility>

using namespace ou;


namespace {

std::optional<std::filesystem::path> FindSettingsPath() {
	const auto folder = FindProjectSettingsFolder();
	if (!folder) { return std::nullopt; }
	return *folder / L"EditorBuildSettings.json";
}

} // namespace



std::vector<EditorBuildSettingsScene> ou::EditorBuildSettings::GetScenes() {
	std::vector<EditorBuildSettingsScene> scenes;

	const auto path = FindSettingsPath();
	if (!path || !std::filesystem::exists(*path)) { return scenes; }

	const auto json = JsonFile::Load(*path);
	if (!json) { return scenes; }

	try {
		const auto it = json->find("scenes");
		if (it == json->end() || !it->is_array()) { return scenes; }

		for (const auto &entry : *it) {
			EditorBuildSettingsScene scene;
			scene.path = entry.value("path", std::string{});
			scene.guid = Guid::Parse(entry.value("guid", std::string{})).value_or(Guid::Invalid);
			scene.enabled = entry.value("enabled", true);

			// シーンを動かしていたら、guid から今のパスにする
			if (scene.guid != Guid::Invalid) {
				if (auto current = AssetDatabase::GuidToAssetPath(scene.guid); !current.empty()) {
					scene.path = std::move(current);
				}
			}
			scenes.push_back(std::move(scene));
		}
	} catch (const Json::exception &e) {
		Log::Error("EditorBuildSettings.json の読み込みに失敗しました: {}\n{}", ToUtf8(*path), e.what());
	}

	return scenes;
}


void ou::EditorBuildSettings::SetScenes(const std::vector<EditorBuildSettingsScene> &scenes) {
	const auto path = FindSettingsPath();
	if (!path) {
		Log::Error("ProjectSettings フォルダが見つからないため、Build Settings を保存できません");
		return;
	}

	auto array = Json::array();
	for (const auto &scene : scenes) {
		auto entry = Json::object();
		entry["path"] = scene.path;
		entry["guid"] = scene.guid.ToString();
		entry["enabled"] = scene.enabled;
		array.push_back(std::move(entry));
	}

	auto root = Json::object();
	root["scenes"] = std::move(array);

	if (!JsonFile::Save(*path, root)) { return; }

	// 実行時の build index を振り直す
	SceneManager::LoadBuildSettings();
}
