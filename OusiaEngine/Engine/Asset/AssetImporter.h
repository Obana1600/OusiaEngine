#pragma once

#include "Scene/Object.h"

#include <filesystem>
#include <memory>

// TODO: API 名を Unity に合わせる

namespace ou {

class AssetImporter: public Object {
public: // メンバ関数

	~AssetImporter() override = default;

	virtual std::shared_ptr<Object> Import() = 0;

	const std::filesystem::path &GetAssetPath() const;


protected: // メンバ関数

	explicit AssetImporter(std::filesystem::path assetPath);


private: // メンバ変数

	std::filesystem::path assetPath_;
};

} // namespace ou
