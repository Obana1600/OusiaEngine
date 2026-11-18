#include "PrefabImporter.h"
#include "Core/JsonFile.h"
#include "Core/StringUtil.h"
#include "Diagnostics/Log.h"
#include "Scene/GameObject.h"
#include "Scene/Scene.h"
#include "Serialize/SceneSerializer.h"

#include <utility>

using namespace ou;


ou::PrefabImporter::PrefabImporter(std::string assetPath, std::filesystem::path fullPath)
	: AssetImporter(std::move(assetPath), std::move(fullPath)) {}


std::shared_ptr<Object> ou::PrefabImporter::Import() {
	return ImportPrefab();
}


std::shared_ptr<GameObject> ou::PrefabImporter::ImportPrefab() {
	const auto json = JsonFile::Load(GetFullPath());
	if (!json || !SceneSerializer::ValidateRoot(*json)) { return nullptr; }

	const auto name = ToUtf8(GetFullPath().stem());
	auto scene = std::make_shared<Scene>(name, -1);
	const auto created = SceneSerializer::CreateGameObjects(*scene, *json, {}, false);
	if (created.empty()) {
		Log::Error("プレハブに GameObject がありません: {}", GetAssetPath());
		return nullptr;
	}

	auto *root = created.front();
	root->SetName(name);

	return std::shared_ptr<GameObject>(std::move(scene), root);
}
