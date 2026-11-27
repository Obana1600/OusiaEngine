#include "SceneRenderTarget.h"
#include "Diagnostics/Fatal.h"
#include "Graphics/GraphicsDevice.h"

#include <algorithm>
#include <cmath>
#include <cstring>
#include <string>
#include <vector>

using namespace ou;


namespace {

constexpr Graphics::Format kSceneColorFormat = Graphics::Format::R11G11B10_FLOAT;
constexpr Graphics::Format kColorFormat = Graphics::Format::B8G8R8A8_UNORM_SRGB;

// TODO: Camera Component に移す
constexpr float kClearColor[4] = {0.1f, 0.25f, 0.5f, 1.0f};

constexpr uint32_t kExposureBinCount = 256;
constexpr uint32_t kExposureBufferSize = kExposureBinCount * 4 + 16;


void CreateColorTexture(uint32_t width, uint32_t height, Graphics::Format format, const std::string &name, Graphics::Texture *texture) {
	auto *device = Graphics::GetDevice();

	Graphics::TextureDesc desc{};
	desc.width = width;
	desc.height = height;
	desc.format = format;
	desc.bindFlags = Graphics::BindFlag::RENDER_TARGET | Graphics::BindFlag::SHADER_RESOURCE;
	desc.layout = Graphics::ResourceState::SHADER_RESOURCE;
	for (int i = 0; i < 4; i++) {
		desc.clear.color[i] = kClearColor[i];
	}

	if (!device->CreateTexture(&desc, nullptr, texture)) {
		FatalError(name + " の作成に失敗しました");
	}
	device->SetName(texture, name);
}

} // namespace



ou::SceneRenderTarget::SceneRenderTarget(std::string_view name)
	: name_(name) {}


void ou::SceneRenderTarget::BeginScenePass(Graphics::CommandList cmd) const {
	using Image = Graphics::RenderPassImage;

	const Image images[] = {
		Image::RenderTarget(&sceneColor_, Image::LoadOp::CLEAR),
		Image::DepthStencil(&depth_, Image::LoadOp::CLEAR),
	};
	Graphics::GetDevice()->RenderPassBegin(images, 2, cmd);
	BindViewport(cmd);
}


void ou::SceneRenderTarget::ResumeScenePass(Graphics::CommandList cmd) const {
	using Image = Graphics::RenderPassImage;

	const Image images[] = {
		Image::RenderTarget(&sceneColor_),
		Image::DepthStencil(&depth_),
	};
	Graphics::GetDevice()->RenderPassBegin(images, 2, cmd);
	BindViewport(cmd);
}


void ou::SceneRenderTarget::BeginDepthCopyPass(Graphics::CommandList cmd) const {
	const auto image = Graphics::RenderPassImage::RenderTarget(&depthCopy_, Graphics::RenderPassImage::LoadOp::DONTCARE);
	Graphics::GetDevice()->RenderPassBegin(&image, 1, cmd);
	BindViewport(cmd);
}


void ou::SceneRenderTarget::BeginOpaqueCopyPass(Graphics::CommandList cmd) const {
	auto *device = Graphics::GetDevice();

	const auto image = Graphics::RenderPassImage::RenderTarget(&opaqueTexture_, Graphics::RenderPassImage::LoadOp::DONTCARE);
	device->RenderPassBegin(&image, 1, cmd);

	const auto &desc = opaqueTexture_.GetDesc();
	Graphics::Viewport viewport{};
	viewport.width = static_cast<float>(desc.width);
	viewport.height = static_cast<float>(desc.height);
	device->BindViewports(1, &viewport, cmd);

	Graphics::Scissor scissor{};
	scissor.right = static_cast<int32_t>(desc.width);
	scissor.bottom = static_cast<int32_t>(desc.height);
	device->BindScissorRects(1, &scissor, cmd);
}


void ou::SceneRenderTarget::BeginPostProcessPass(Graphics::CommandList cmd) const {
	using Image = Graphics::RenderPassImage;

	const auto image = Image::RenderTarget(&color_, Image::LoadOp::DONTCARE);
	Graphics::GetDevice()->RenderPassBegin(&image, 1, cmd);
	BindViewport(cmd);
}


void ou::SceneRenderTarget::BeginOverlayPass(Graphics::CommandList cmd) const {
	const auto image = Graphics::RenderPassImage::RenderTarget(&color_);
	Graphics::GetDevice()->RenderPassBegin(&image, 1, cmd);
	BindViewport(cmd);
}


void ou::SceneRenderTarget::BeginOverlayPassWithDepth(Graphics::CommandList cmd) const {
	using Image = Graphics::RenderPassImage;

	const Image images[] = {
		Image::RenderTarget(&color_),
		Image::DepthStencil(&depth_, Image::LoadOp::LOAD, Image::StoreOp::DONTCARE),
	};
	Graphics::GetDevice()->RenderPassBegin(images, 2, cmd);
	BindViewport(cmd);
}


void ou::SceneRenderTarget::Resize(uint32_t width, uint32_t height) {
	if (width == 0 || height == 0) { return; }

	// 不透明テクスチャ
	const auto downsampling = GraphicsSettings::GetInstance().opaqueDownsampling_;
	if (width == width_ && height == height_) {
		if (downsampling != opaqueDownsampling_) { CreateOpaqueTexture(downsampling); }
		return;
	}

	// 露出のバッファ
	if (!exposureBuffer_.IsValid()) { CreateExposureBuffer(); }

	auto *device = Graphics::GetDevice();

	CreateColorTexture(width, height, kSceneColorFormat, name_ + "_SceneColor", &sceneColor_);
	CreateColorTexture(width, height, kColorFormat, name_ + "_Color", &color_);
	CreateColorTexture(width, height, kSceneColorFormat, name_ + "_SceneColorCopy", &sceneColorCopy_);
	CreateColorTexture(width, height, kColorFormat, name_ + "_ColorCopy", &colorCopy_);
	CreateColorTexture(width, height, Graphics::Format::R32_FLOAT, name_ + "_DepthCopy", &depthCopy_);

	Graphics::TextureDesc depthDesc{};
	depthDesc.width = width;
	depthDesc.height = height;
	depthDesc.format = Graphics::Format::D32_FLOAT;
	depthDesc.bindFlags = Graphics::BindFlag::DEPTH_STENCIL | Graphics::BindFlag::SHADER_RESOURCE;
	depthDesc.layout = Graphics::ResourceState::DEPTHSTENCIL;
	depthDesc.clear.depthStencil.depth = 1.0f;
	if (!device->CreateTexture(&depthDesc, nullptr, &depth_)) {
		FatalError(name_ + " の深度のテクスチャの作成に失敗しました");
	}
	device->SetName(&depth_, name_ + "Depth");

	const auto halfSize = std::max(std::max(width / 2, height / 2), 1u);
	const auto iterations = static_cast<int>(std::floor(std::log2(static_cast<float>(halfSize)))) - 1;
	bloomMipCount_ = static_cast<uint32_t>(std::clamp(iterations, 1, static_cast<int>(kMaxBloomMipCount)));
	for (uint32_t i = 0; i < kMaxBloomMipCount; i++) {
		if (i >= bloomMipCount_) {
			bloomMipDown_[i] = {};
			bloomMipUp_[i] = {};
			continue;
		}

		const auto mipWidth = std::max(width >> (i + 1), 1u);
		const auto mipHeight = std::max(height >> (i + 1), 1u);
		const auto number = std::to_string(i);
		CreateColorTexture(mipWidth, mipHeight, kSceneColorFormat, name_ + "_BloomMipDown" + number, &bloomMipDown_[i]);
		CreateColorTexture(mipWidth, mipHeight, kSceneColorFormat, name_ + "_BloomMipUp" + number, &bloomMipUp_[i]);
	}

	width_ = width;
	height_ = height;

	CreateOpaqueTexture(downsampling);
}


void ou::SceneRenderTarget::Release() {
	sceneColor_ = {};
	color_ = {};
	sceneColorCopy_ = {};
	colorCopy_ = {};
	depth_ = {};
	depthCopy_ = {};
	opaqueTexture_ = {};
	opaqueDownsampling_ = Downsampling::None;
	width_ = 0;
	height_ = 0;

	bloomMipCount_ = 0;
	bloomMipDown_.fill({});
	bloomMipUp_.fill({});
	exposureBuffer_ = {};
}


const Graphics::Texture &ou::SceneRenderTarget::GetSceneColor() const {
	return sceneColor_;
}


const Graphics::Texture &ou::SceneRenderTarget::GetColor() const {
	return color_;
}


const Graphics::Texture &ou::SceneRenderTarget::GetSceneColorCopy() const {
	return sceneColorCopy_;
}


const Graphics::Texture &ou::SceneRenderTarget::GetColorCopy() const {
	return colorCopy_;
}


const Graphics::Texture &ou::SceneRenderTarget::GetDepth() const {
	return depth_;
}


const Graphics::Texture &ou::SceneRenderTarget::GetDepthCopy() const {
	return depthCopy_;
}


const Graphics::Texture &ou::SceneRenderTarget::GetOpaqueTexture() const {
	return opaqueTexture_;
}


uint32_t ou::SceneRenderTarget::GetWidth() const {
	return width_;
}


uint32_t ou::SceneRenderTarget::GetHeight() const {
	return height_;
}


bool ou::SceneRenderTarget::IsCreated() const {
	return width_ > 0 && height_ > 0;
}


uint32_t ou::SceneRenderTarget::GetBloomMipCount() const {
	return bloomMipCount_;
}


const Graphics::Texture &ou::SceneRenderTarget::GetBloomMipDown(uint32_t index) const {
	return bloomMipDown_[index]; // TODO: 範囲外アクセス
}


const Graphics::Texture &ou::SceneRenderTarget::GetBloomMipUp(uint32_t index) const {
	return bloomMipUp_[index]; // TODO: 範囲外アクセス
}


const Graphics::GpuBuffer &ou::SceneRenderTarget::GetExposureBuffer() const {
	return exposureBuffer_;
}


void ou::SceneRenderTarget::BindViewport(Graphics::CommandList cmd) const {
	auto *device = Graphics::GetDevice();

	Graphics::Viewport viewport{};
	viewport.width = static_cast<float>(width_);
	viewport.height = static_cast<float>(height_);
	device->BindViewports(1, &viewport, cmd);

	Graphics::Scissor scissor{};
	scissor.right = static_cast<int32_t>(width_);
	scissor.bottom = static_cast<int32_t>(height_);
	device->BindScissorRects(1, &scissor, cmd);
}


void ou::SceneRenderTarget::CreateOpaqueTexture(Downsampling downsampling) {
	const auto divisor = (downsampling == Downsampling::_2xBilinear) ? 2u : 1u;
	const auto width = std::max(width_ / divisor, 1u);
	const auto height = std::max(height_ / divisor, 1u);

	CreateColorTexture(width, height, kSceneColorFormat, name_ + "_OpaqueTexture", &opaqueTexture_);
	opaqueDownsampling_ = downsampling;
}


void ou::SceneRenderTarget::CreateExposureBuffer() {
	std::vector<uint32_t> initialData(kExposureBufferSize / 4, 0);
	const float exposure = 1.0f;
	std::memcpy(&initialData[kExposureBinCount], &exposure, sizeof(exposure));

	Graphics::GpuBufferDesc desc{};
	desc.size = kExposureBufferSize;
	desc.bindFlags = Graphics::BindFlag::SHADER_RESOURCE | Graphics::BindFlag::UNORDERED_ACCESS;
	desc.miscFlags = Graphics::ResourceMiscFlag::BUFFER_RAW;

	auto *device = Graphics::GetDevice();
	if (!device->CreateBuffer(&desc, initialData.data(), &exposureBuffer_)) {
		FatalError(name_ + " の露出のバッファの作成に失敗しました");
	}
	device->SetName(&exposureBuffer_, name_ + "_Exposure");
}
