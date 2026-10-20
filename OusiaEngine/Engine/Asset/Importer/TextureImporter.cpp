#include "TextureImporter.h"
#include "Core/StringUtil.h"
#include "Diagnostics/Log.h"
#include "Graphics/GraphicsDevice.h"
#include "Render/Texture.h"

#include <DirectXTex/DirectXTex.h>
#include <optional>
#include <utility>
#include <vector>

using namespace ou;


namespace {

std::optional<DirectX::ScratchImage> LoadScratchImage(
	const std::filesystem::path &path, bool srgb, bool mipmapEnabled) {
	const auto pathUtf8 = ToUtf8(path);

	// テクスチャの読み込み
	DirectX::ScratchImage image;
	const auto wicFlags = srgb ? DirectX::WIC_FLAGS_FORCE_SRGB : DirectX::WIC_FLAGS_IGNORE_SRGB;
	auto hr = DirectX::LoadFromWICFile(path.c_str(), wicFlags, nullptr, image);
	if (FAILED(hr)) {
		Log::Error("テクスチャを読めませんでした: {} (hr = {:#010x})",
				   pathUtf8, static_cast<uint32_t>(hr));
		return std::nullopt;
	}

	// フォーマットをそろえる
	const auto format = srgb ? DXGI_FORMAT_R8G8B8A8_UNORM_SRGB : DXGI_FORMAT_R8G8B8A8_UNORM;
	if (image.GetMetadata().format != format) {
		DirectX::ScratchImage converted;
		hr = DirectX::Convert(
			image.GetImages(), image.GetImageCount(), image.GetMetadata(),
			format, DirectX::TEX_FILTER_DEFAULT, DirectX::TEX_THRESHOLD_DEFAULT, converted);
		if (FAILED(hr)) {
			Log::Error("テクスチャのフォーマットを変換できませんでした: {} (hr = {:#010x})",
					   pathUtf8, static_cast<uint32_t>(hr));
			return std::nullopt;
		}
		image = std::move(converted);
	}

	// 1x1 はミップを作れない
	const auto &meta = image.GetMetadata();
	if (!mipmapEnabled || (meta.width <= 1 && meta.height <= 1)) {
		return image;
	}

	// ミップマップの作成
	DirectX::ScratchImage mipImages;
	hr = DirectX::GenerateMipMaps(
		image.GetImages(), image.GetImageCount(), meta,
		(srgb ? DirectX::TEX_FILTER_SRGB : DirectX::TEX_FILTER_DEFAULT), 0, mipImages);
	if (FAILED(hr)) {
		Log::Error("ミップマップを作れませんでした: {} (hr = {:#010x})",
				   pathUtf8, static_cast<uint32_t>(hr));
		return std::nullopt;
	}

	return mipImages;
}

} // namespace



ou::TextureImporter::TextureImporter(std::string assetPath, std::filesystem::path fullPath)
	: AssetImporter(std::move(assetPath), std::move(fullPath)) {}


std::shared_ptr<Object> ou::TextureImporter::Import() {
	return ImportTexture();
}


std::shared_ptr<Texture> ou::TextureImporter::ImportTexture() {
	const auto image = LoadScratchImage(GetFullPath(), srgb_, mipmapEnabled_);
	if (!image) { return nullptr; }

	const auto &meta = image->GetMetadata();

	// グラフィクス層のテクスチャの設定
	Graphics::TextureDesc desc{};
	desc.width = static_cast<uint32_t>(meta.width);
	desc.height = static_cast<uint32_t>(meta.height);
	desc.arraySize = static_cast<uint32_t>(meta.arraySize);
	desc.mipLevels = static_cast<uint32_t>(meta.mipLevels);
	desc.format = srgb_ ? Graphics::Format::R8G8B8A8_UNORM_SRGB : Graphics::Format::R8G8B8A8_UNORM;
	desc.bindFlags = Graphics::BindFlag::SHADER_RESOURCE;
	desc.layout = Graphics::ResourceState::SHADER_RESOURCE;

	// ScratchImage のデータを配列にする
	std::vector<Graphics::SubresourceData> subresources(image->GetImageCount());
	for (size_t i = 0; i < subresources.size(); i++) {
		const auto &source = image->GetImages()[i];
		subresources[i].dataPtr = source.pixels;
		subresources[i].rowPitch = static_cast<uint32_t>(source.rowPitch);
		subresources[i].slicePitch = static_cast<uint32_t>(source.slicePitch);
	}

	// グラフィクス層のテクスチャの作成
	Graphics::Texture graphicsTexture;
	if (!Graphics::GetDevice()->CreateTexture(&desc, subresources.data(), &graphicsTexture)) {
		Log::Error("テクスチャの作成に失敗しました: {}", ToUtf8(GetFullPath()));
		return nullptr;
	}
	Graphics::GetDevice()->SetName(&graphicsTexture, GetName());

	// アセット層のテクスチャの作成
	auto texture = std::make_shared<Texture>(std::move(graphicsTexture));
	texture->SetName(GetName());
	return texture;
}


void ou::TextureImporter::SetSrgb(bool srgb) {
	srgb_ = srgb;
}


void ou::TextureImporter::SetMipmapEnabled(bool mipmapEnabled) {
	mipmapEnabled_ = mipmapEnabled;
}
