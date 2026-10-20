#pragma once

#include "Core/Object.h"
#include "Graphics/GraphicsTypes.h"

#include <cstdint>

// TODO: Unity のように Texture2D / RenderTexture / CubeMap... と派生させる

namespace ou {

class Texture: public Object {
public: // メンバ関数

	explicit Texture(Graphics::Texture graphicsTexture);

	const Graphics::Texture &GetGraphicsTexture() const;

	uint32_t GetWidth() const;
	uint32_t GetHeight() const;
	uint32_t GetDescriptorIndex() const;


private: // メンバ変数

	Graphics::Texture graphicsTexture_;
};

} // namespace ou
