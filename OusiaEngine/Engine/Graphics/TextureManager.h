#pragma once

#include "Graphics/GraphicsTypes.h"

#include <cstdint>
#include <string>
#include <string_view>
#include <vector>


namespace ou {

class TextureManager {
public: // 静的メンバ関数

	// TODO: シングルトン辞めたい
	static TextureManager *GetInstance();

	static uint32_t Load(std::string_view fileName);


public: // メンバ関数

	void Finalize();

	const Graphics::Texture &GetTexture(uint32_t textureHandle) const;

	uint32_t GetDescriptorIndex(uint32_t textureHandle) const;


private: // インナークラス

	struct Entry {
		Graphics::Texture texture;
		std::string name;
	};


private: // メンバ変数

	std::vector<Entry> textures_;


private: // メンバ関数

	uint32_t LoadInternal(std::string_view fileName);


private: // コンストラクタ

	TextureManager() = default;
	~TextureManager() = default;
	TextureManager(const TextureManager &) = delete;
	TextureManager &operator=(const TextureManager &) = delete;
};

} // namespace ou
