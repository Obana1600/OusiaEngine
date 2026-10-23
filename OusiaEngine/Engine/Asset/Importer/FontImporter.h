#pragma once

#include "Asset/AssetImporter.h"

#include <cstdint>
#include <filesystem>
#include <memory>
#include <string>


namespace ou {

// 前方宣言
class FontAsset;

class FontImporter: public AssetImporter {
public: // メンバ関数

	explicit FontImporter(std::string assetPath, std::filesystem::path fullPath);

	std::shared_ptr<Object> Import() override;
	std::shared_ptr<FontAsset> ImportFontAsset();

	void SetSamplingPointSize(uint32_t samplingPointSize);
	void SetPadding(uint32_t padding);
	void SetAtlasSize(uint32_t atlasSize);


private: // メンバ変数

	uint32_t samplingPointSize_ = 64;
	uint32_t padding_ = 8;
	uint32_t atlasSize_ = 2048;
};

} // namespace ou
