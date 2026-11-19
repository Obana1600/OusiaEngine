#pragma once

#include "Core/Object.h"

#include <filesystem>
#include <memory>
#include <string>
#include <string_view>
#include <vector>

// TODO: API 名を Unity に合わせる

namespace ou {

class AssetImporter: public Object {
public: // インナークラス

	struct SubAsset {
		std::string name; // "型/名前"
		std::weak_ptr<Object> object;
	};


public: // メンバ関数

	~AssetImporter() override = default;

	virtual std::shared_ptr<Object> Import() = 0;

	const std::string &GetAssetPath() const;
	const std::filesystem::path &GetFullPath() const;

	// ファイルを動かした・名前を変えたときに、AssetDatabase::MoveAsset から呼ぶ
	void SetPath(std::string assetPath, std::filesystem::path fullPath);

	std::string ResolveAssetPath(std::string_view relativePath) const;

	const std::vector<SubAsset> &GetSubAssets() const;
	void ClearSubAssets();

#ifdef WITH_EDITOR
	void SaveAndReimport();
#endif


protected: // メンバ関数

	explicit AssetImporter(std::string assetPath, std::filesystem::path fullPath);

	void AddSubAsset(std::string_view typeName, const std::shared_ptr<Object> &object);

	void ReadImportSettings();


private: // メンバ変数

	std::string assetPath_;
	std::filesystem::path fullPath_;

	std::vector<SubAsset> subAssets_;
};

} // namespace ou
