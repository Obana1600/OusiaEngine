#pragma once

#include "Asset/AssetImporter.h"

#include <filesystem>
#include <memory>


namespace ou {

// 前方宣言
class Texture;

class TextureImporter: public AssetImporter {
public: // メンバ関数

	explicit TextureImporter(std::string assetPath, std::filesystem::path fullPath);

	std::shared_ptr<Object> Import() override;
	std::shared_ptr<Texture> ImportTexture();

	void SetSrgb(bool srgb);
	void SetMipmapEnabled(bool mipmapEnabled);


private: // メンバ変数

	bool srgb_ = true;
	bool mipmapEnabled_ = true;
};

} // namespace ou
