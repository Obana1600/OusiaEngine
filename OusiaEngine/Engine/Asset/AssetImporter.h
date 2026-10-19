#pragma once

#include "Scene/Object.h"

#include <filesystem>
#include <memory>
#include <string>
#include <string_view>

// TODO: API 名を Unity に合わせる

namespace ou {

class AssetImporter: public Object {
public: // メンバ関数

	~AssetImporter() override = default;

	virtual std::shared_ptr<Object> Import() = 0;

	const std::string &GetAssetPath() const;
	const std::filesystem::path &GetFullPath() const;

	std::string ResolveAssetPath(std::string_view relativePath) const;


protected: // メンバ関数

	explicit AssetImporter(std::string assetPath, std::filesystem::path fullPath);


private: // メンバ変数

	std::string assetPath_;
	std::filesystem::path fullPath_;
};

} // namespace ou
