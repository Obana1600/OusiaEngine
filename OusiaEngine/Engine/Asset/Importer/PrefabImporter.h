#pragma once

#include "Asset/AssetImporter.h"

#include <filesystem>
#include <memory>
#include <string>


namespace ou {

class PrefabImporter final: public AssetImporter {
public: // メンバ関数

	explicit PrefabImporter(std::string assetPath, std::filesystem::path fullPath);

	std::shared_ptr<Object> Import() override;
	std::shared_ptr<GameObject> ImportPrefab();
};

} // namespace ou
