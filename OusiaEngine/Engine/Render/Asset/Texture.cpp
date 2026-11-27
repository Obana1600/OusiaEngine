#include "Texture.h"
#include "Graphics/GraphicsDevice.h"

#include <utility>

using namespace ou;


ou::Texture::Texture(Graphics::Texture graphicsTexture)
	: graphicsTexture_(std::move(graphicsTexture)) {}


uint32_t ou::Texture::GetWidth() const {
	return graphicsTexture_.desc.width;
}


uint32_t ou::Texture::GetHeight() const {
	return graphicsTexture_.desc.height;
}


const Graphics::Texture &ou::Texture::GetGraphicsTexture() const {
	return graphicsTexture_;
}


uint32_t ou::Texture::GetDescriptorIndex() const {
	const auto index = Graphics::GetDevice()->GetDescriptorIndex(&graphicsTexture_, Graphics::ViewType::SRV);
	return static_cast<uint32_t>(index);
}


bool ou::Texture::IsReadable() const {
	return !pixels_.empty();
}
