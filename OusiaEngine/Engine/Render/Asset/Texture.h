#pragma once

#include "Core/Object.h"
#include "Graphics/GraphicsTypes.h"

#include <cstdint>
#include <vector>

// TODO: Texture2D / CubeMap も作る

namespace ou {

class Texture: public Object {
	friend class TextureImporter;
	friend class Cursor;

public: // メンバ関数

	explicit Texture(Graphics::Texture graphicsTexture);

	virtual uint32_t GetWidth() const;
	virtual uint32_t GetHeight() const;

	const Graphics::Texture &GetGraphicsTexture() const;
	uint32_t GetDescriptorIndex() const;
	bool IsReadable() const;


protected: // メンバ関数

	Texture() = default;


protected: // メンバ変数

	Graphics::Texture graphicsTexture_;


private: // メンバ変数

	std::vector<uint8_t> pixels_;
};

} // namespace ou
