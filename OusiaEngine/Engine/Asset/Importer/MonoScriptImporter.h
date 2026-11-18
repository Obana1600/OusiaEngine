#pragma once

#include "Asset/AssetImporter.h"

#include <filesystem>
#include <memory>
#include <string>


namespace ou {

class MonoScriptImporter final: public AssetImporter {
public: // メンバ関数

	explicit MonoScriptImporter(std::string assetPath, std::filesystem::path fullPath);

	std::shared_ptr<Object> Import() override;
};

} // namespace ou
