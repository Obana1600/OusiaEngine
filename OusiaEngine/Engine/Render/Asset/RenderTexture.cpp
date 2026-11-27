#include "RenderTexture.h"
#include "Diagnostics/Log.h"
#include "Graphics/GraphicsDevice.h"

#include <magic_enum/magic_enum.hpp>

using namespace ou;


namespace {

Graphics::Format ToGraphicsFormat(RenderTextureFormat format) {
	switch (format) {
		case RenderTextureFormat::ARGB32:
		case RenderTextureFormat::Default:
			return Graphics::Format::R8G8B8A8_UNORM;

		case RenderTextureFormat::ARGBHalf:
		case RenderTextureFormat::DefaultHDR:
			return Graphics::Format::R16G16B16A16_FLOAT;

		case RenderTextureFormat::ARGBFloat:
			return Graphics::Format::R32G32B32A32_FLOAT;

		case RenderTextureFormat::RGFloat:
			return Graphics::Format::R32G32_FLOAT;

		case RenderTextureFormat::RFloat:
			return Graphics::Format::R32_FLOAT;

		case RenderTextureFormat::R8:
			return Graphics::Format::R8_UNORM;

		case RenderTextureFormat::BGRA32:
			return Graphics::Format::B8G8R8A8_UNORM;

		case RenderTextureFormat::RGB111110Float:
			return Graphics::Format::R11G11B10_FLOAT;
	}
	return Graphics::Format::UNKNOWN;
}

} // namespace



ou::RenderTexture::RenderTexture(int width, int height, int depth, RenderTextureFormat format)
	: width_(width),
	  height_(height),
	  format_(format) {
	if (depth != 0) {
		Log::Warning("RenderTexture の深度バッファにはまだ対応していません (depth={})", depth);
	}
}


bool ou::RenderTexture::Create() {
	if (IsCreated()) { return true; }

	if (width_ <= 0 || height_ <= 0) {
		Log::Error("RenderTexture \"{}\" の大きさは 1 以上にしてください ({}x{})", GetName(), width_, height_);
		return false;
	}

	const auto format = ToGraphicsFormat(format_);
	if (format == Graphics::Format::UNKNOWN) {
		Log::Error("RenderTexture \"{}\" の形式 {} にはまだ対応していません", GetName(), magic_enum::enum_name(format_));
		return false;
	}

	Graphics::TextureDesc desc{};
	desc.width = static_cast<uint32_t>(width_);
	desc.height = static_cast<uint32_t>(height_);
	desc.format = format;
	desc.bindFlags = Graphics::BindFlag::RENDER_TARGET | Graphics::BindFlag::SHADER_RESOURCE;
	if (enableRandomWrite_) { desc.bindFlags |= Graphics::BindFlag::UNORDERED_ACCESS; }
	desc.layout = Graphics::ResourceState::SHADER_RESOURCE;

	auto *device = Graphics::GetDevice();
	if (!device->CreateTexture(&desc, nullptr, &graphicsTexture_)) {
		Log::Error("RenderTexture \"{}\" の作成に失敗しました", GetName());
		graphicsTexture_ = {};
		return false;
	}
	device->SetName(&graphicsTexture_, GetName().empty() ? "RenderTexture" : GetName());
	return true;
}


void ou::RenderTexture::Release() {
	graphicsTexture_ = {};
}


bool ou::RenderTexture::IsCreated() const {
	return graphicsTexture_.IsValid();
}


void ou::RenderTexture::SetEnableRandomWrite(bool enableRandomWrite) {
	if (IsCreated()) {
		Log::Error("作成済みの RenderTexture \"{}\" の enableRandomWrite は変えられません。Release してから変えてください", GetName());
		return;
	}
	enableRandomWrite_ = enableRandomWrite;
}


bool ou::RenderTexture::GetEnableRandomWrite() const {
	return enableRandomWrite_;
}


RenderTextureFormat ou::RenderTexture::GetFormat() const {
	return format_;
}


uint32_t ou::RenderTexture::GetWidth() const {
	return static_cast<uint32_t>(width_);
}


uint32_t ou::RenderTexture::GetHeight() const {
	return static_cast<uint32_t>(height_);
}
