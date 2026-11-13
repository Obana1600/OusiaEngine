#include "SceneRenderTarget.h"
#include "Diagnostics/Fatal.h"
#include "Graphics/GraphicsDevice.h"

using namespace ou;


namespace {

constexpr float kClearColor[4] = {0.1f, 0.25f, 0.5f, 1.0f};

} // namespace



ou::SceneRenderTarget::SceneRenderTarget(std::string_view name)
	: name_(name) {}


void ou::SceneRenderTarget::BeginScenePass(Graphics::CommandList cmd) const {
	using Image = Graphics::RenderPassImage;
	const Image images[] = {
		Image::RenderTarget(&color_, Image::LoadOp::CLEAR),
		Image::DepthStencil(&depth_, Image::LoadOp::CLEAR, Image::StoreOp::DONTCARE),
	};
	Graphics::GetDevice()->RenderPassBegin(images, 2, cmd);
	BindViewport(cmd);
}


void ou::SceneRenderTarget::BeginOverlayPass(Graphics::CommandList cmd) const {
	const auto image = Graphics::RenderPassImage::RenderTarget(&color_);
	Graphics::GetDevice()->RenderPassBegin(&image, 1, cmd);
	BindViewport(cmd);
}


void ou::SceneRenderTarget::Resize(uint32_t width, uint32_t height) {
	if (width == 0 || height == 0) { return; }
	if (width == width_ && height == height_) { return; }

	auto *device = Graphics::GetDevice();

	Graphics::TextureDesc colorDesc{};
	colorDesc.width = width;
	colorDesc.height = height;
	colorDesc.format = Graphics::Format::B8G8R8A8_UNORM_SRGB;
	colorDesc.bindFlags = Graphics::BindFlag::RENDER_TARGET | Graphics::BindFlag::SHADER_RESOURCE;
	colorDesc.layout = Graphics::ResourceState::SHADER_RESOURCE;
	for (int i = 0; i < 4; i++) {
		colorDesc.clear.color[i] = kClearColor[i];
	}
	if (!device->CreateTexture(&colorDesc, nullptr, &color_)) {
		FatalError(name_ + " の色のテクスチャの作成に失敗しました");
	}
	device->SetName(&color_, name_ + "Color");

	Graphics::TextureDesc depthDesc{};
	depthDesc.width = width;
	depthDesc.height = height;
	depthDesc.format = Graphics::Format::D32_FLOAT;
	depthDesc.bindFlags = Graphics::BindFlag::DEPTH_STENCIL;
	depthDesc.layout = Graphics::ResourceState::DEPTHSTENCIL;
	depthDesc.clear.depthStencil.depth = 1.0f;
	if (!device->CreateTexture(&depthDesc, nullptr, &depth_)) {
		FatalError(name_ + " の深度のテクスチャの作成に失敗しました");
	}
	device->SetName(&depth_, name_ + "Depth");

	width_ = width;
	height_ = height;
}


void ou::SceneRenderTarget::Release() {
	color_ = {};
	depth_ = {};
	width_ = 0;
	height_ = 0;
}


const Graphics::Texture &ou::SceneRenderTarget::GetColor() const {
	return color_;
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
