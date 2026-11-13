#pragma once

#include "Graphics/GraphicsTypes.h"

#include <cstdint>
#include <string>
#include <string_view>


namespace ou {

class SceneRenderTarget final {
public: // メンバ関数

	explicit SceneRenderTarget(std::string_view name);

	void BeginScenePass(Graphics::CommandList cmd) const;
	void BeginOverlayPass(Graphics::CommandList cmd) const;

	void Resize(uint32_t width, uint32_t height);
	void Release();

	const Graphics::Texture &GetColor() const;
	uint32_t GetWidth() const;
	uint32_t GetHeight() const;
	bool IsCreated() const;


private: // メンバ変数

	std::string name_;
	Graphics::Texture color_;
	Graphics::Texture depth_;
	uint32_t width_ = 0;
	uint32_t height_ = 0;


private: // メンバ関数

	void BindViewport(Graphics::CommandList cmd) const;
};

} // namespace ou
