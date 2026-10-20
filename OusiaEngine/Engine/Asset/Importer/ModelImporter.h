#pragma once

#include "Asset/AssetImporter.h"

#include <filesystem>
#include <memory>
#include <string>


namespace ou {

// 前方宣言
class Model;

class ModelImporter: public AssetImporter {
public: // メンバ関数

	explicit ModelImporter(std::string assetPath, std::filesystem::path fullPath);

	std::shared_ptr<Object> Import() override;
	std::shared_ptr<Model> ImportModel();

	void SetGlobalScale(float globalScale);
	float GetGlobalScale() const;


private: // メンバ変数

	float globalScale_ = 1.0f;
};

} // namespace ou
