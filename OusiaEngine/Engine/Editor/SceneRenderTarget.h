#pragma once

#include "Graphics/GraphicsTypes.h"
#include "Render/GraphicsSettings.h"

#include <array>
#include <cstdint>
#include <string>
#include <string_view>


namespace ou {

class SceneRenderTarget final {
public: // 静的メンバ変数

	// Bloom の縮小の段の数の上限
	static constexpr uint32_t kMaxBloomMipCount = 6;


public: // メンバ関数

	explicit SceneRenderTarget(std::string_view name);

	void BeginScenePass(Graphics::CommandList cmd) const;
	void ResumeScenePass(Graphics::CommandList cmd) const;
	void BeginDepthCopyPass(Graphics::CommandList cmd) const;
	void BeginOpaqueCopyPass(Graphics::CommandList cmd) const;
	void BeginPostProcessPass(Graphics::CommandList cmd) const;
	void BeginOverlayPass(Graphics::CommandList cmd) const;
	void BeginOverlayPassWithDepth(Graphics::CommandList cmd) const;

	void Resize(uint32_t width, uint32_t height);
	void Release();

	const Graphics::Texture &GetSceneColor() const;
	const Graphics::Texture &GetColor() const;
	const Graphics::Texture &GetSceneColorCopy() const;
	const Graphics::Texture &GetColorCopy() const;
	const Graphics::Texture &GetDepth() const;
	const Graphics::Texture &GetDepthCopy() const;
	const Graphics::Texture &GetOpaqueTexture() const;
	uint32_t GetWidth() const;
	uint32_t GetHeight() const;
	bool IsCreated() const;

	uint32_t GetBloomMipCount() const;
	const Graphics::Texture &GetBloomMipDown(uint32_t index) const;
	const Graphics::Texture &GetBloomMipUp(uint32_t index) const;


private: // メンバ変数

	std::string name_;
	Graphics::Texture sceneColor_; // HDR
	Graphics::Texture color_;	   // LDR
	Graphics::Texture sceneColorCopy_;
	Graphics::Texture colorCopy_;
	Graphics::Texture depth_;
	Graphics::Texture depthCopy_;
	Graphics::Texture opaqueTexture_;
	Downsampling opaqueDownsampling_ = Downsampling::None;
	uint32_t width_ = 0;
	uint32_t height_ = 0;

	uint32_t bloomMipCount_ = 0;
	std::array<Graphics::Texture, kMaxBloomMipCount> bloomMipDown_;
	std::array<Graphics::Texture, kMaxBloomMipCount> bloomMipUp_;


private: // メンバ関数

	void BindViewport(Graphics::CommandList cmd) const;

	void CreateOpaqueTexture(Downsampling downsampling);
};

} // namespace ou
