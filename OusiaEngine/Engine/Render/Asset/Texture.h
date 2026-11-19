#pragma once

#include "Core/Object.h"
#include "Graphics/GraphicsTypes.h"

#include <cstdint>
#include <vector>

// TODO: Unity のように Texture2D / RenderTexture / CubeMap... と派生させる

namespace ou {

class Texture: public Object {
	friend class TextureImporter;
	friend class Cursor;

public: // メンバ関数

	explicit Texture(Graphics::Texture graphicsTexture);

	const Graphics::Texture &GetGraphicsTexture() const;

	uint32_t GetWidth() const;
	uint32_t GetHeight() const;
	uint32_t GetDescriptorIndex() const;
	bool IsReadable() const;


private: // メンバ変数

	Graphics::Texture graphicsTexture_;
	std::vector<uint8_t> pixels_;
};

} // namespace ou
