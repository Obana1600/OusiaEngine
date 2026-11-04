#pragma once

#include "Asset/AssetImporter.h"

#include <filesystem>
#include <memory>
#include <string>


namespace ou {

// 前方宣言
class PhysicsMaterial;

class PhysicsMaterialImporter final: public AssetImporter {
public: // メンバ関数

	explicit PhysicsMaterialImporter(std::string assetPath, std::filesystem::path fullPath);

	std::shared_ptr<Object> Import() override;
	std::shared_ptr<PhysicsMaterial> ImportPhysicsMaterial();
};

} // namespace ou
