#pragma once

#include "Asset/AssetImporter.h"

#include <filesystem>
#include <memory>
#include <string>


namespace ou {

// 前方宣言
class Texture;

enum class TextureImporterType {
	Default, // 3D モデルや UI の絵
	Cursor,	 // Cursor::SetCursor に渡す
};


class TextureImporter: public AssetImporter {
public: // メンバ関数

	explicit TextureImporter(std::string assetPath, std::filesystem::path fullPath);

	std::shared_ptr<Object> Import() override;
	std::shared_ptr<Texture> ImportTexture();

	void Serialize(Serializer &serializer) override;

	void SetTextureType(TextureImporterType textureType);
	TextureImporterType GetTextureType() const;

	void SetSrgb(bool srgb);
	void SetMipmapEnabled(bool mipmapEnabled);


private: // メンバ変数

	TextureImporterType textureType_ = TextureImporterType::Default;
	bool srgb_ = true;
	bool mipmapEnabled_ = true;
};

} // namespace ou
