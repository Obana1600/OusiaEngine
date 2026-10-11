#include "TextureManager.h"
#include "Core/Fatal.h"
#include "Core/Paths.h"
#include "Graphics/GraphicsDevice.h"

#include <DirectXTex/DirectXTex.h>
#include <filesystem>

using namespace ou;


namespace {

DirectX::ScratchImage LoadImageWithMips(const std::filesystem::path &filePath) {
	DirectX::ScratchImage image{};
	auto hr = DirectX::LoadFromWICFile(filePath.c_str(), DirectX::WIC_FLAGS_FORCE_SRGB, nullptr, image);
	CheckHResult(hr, "テクスチャの読み込み");

	// フォーマットをそろえる
	if (image.GetMetadata().format != DXGI_FORMAT_R8G8B8A8_UNORM_SRGB) {
		DirectX::ScratchImage converted{};
		hr = DirectX::Convert(
			image.GetImages(), image.GetImageCount(), image.GetMetadata(),
			DXGI_FORMAT_R8G8B8A8_UNORM_SRGB, DirectX::TEX_FILTER_DEFAULT, DirectX::TEX_THRESHOLD_DEFAULT, converted);
		CheckHResult(hr, "テクスチャのフォーマット変換");
		image = std::move(converted);
	}

	// 1x1 のテクスチャ
	if (image.GetMetadata().width <= 1 && image.GetMetadata().height <= 1) {
		return image;
	}

	DirectX::ScratchImage mipImages{};
	hr = DirectX::GenerateMipMaps(
		image.GetImages(), image.GetImageCount(), image.GetMetadata(),
		DirectX::TEX_FILTER_SRGB, 0, mipImages);
	CheckHResult(hr, "ミップマップの作成");

	return mipImages;
}

} // namespace



TextureManager *ou::TextureManager::GetInstance() {
	static TextureManager instance;
	return &instance;
}


uint32_t ou::TextureManager::Load(std::string_view fileName) {
	return GetInstance()->LoadInternal(fileName);
}


void ou::TextureManager::Finalize() {
	textures_.clear();
}


const Texture &ou::TextureManager::GetTexture(uint32_t textureHandle) const {
	return textures_[textureHandle].texture;
}


uint32_t ou::TextureManager::GetDescriptorIndex(uint32_t textureHandle) const {
	auto index = GetDevice()->GetDescriptorIndex(&textures_[textureHandle].texture, ViewType::SRV);
	return static_cast<uint32_t>(index);
}


uint32_t ou::TextureManager::LoadInternal(std::string_view fileName) {
	// 読み込み済みならそのハンドルを返す
	for (size_t i = 0; i < textures_.size(); i++) {
		if (textures_[i].name == fileName) {
			return static_cast<uint32_t>(i);
		}
	}

	const auto assetPath = FindAssetsFolder();
	if (!assetPath) {
		FatalError("Assets フォルダが見つかりません。");
	}

	const auto mipImages = LoadImageWithMips(*assetPath / Utf8ToWide(fileName));
	const auto &metaData = mipImages.GetMetadata();

	TextureDesc desc{};
	desc.width = static_cast<uint32_t>(metaData.width);
	desc.height = static_cast<uint32_t>(metaData.height);
	desc.arraySize = static_cast<uint32_t>(metaData.arraySize);
	desc.mipLevels = static_cast<uint32_t>(metaData.mipLevels);
	desc.format = Format::R8G8B8A8_UNORM_SRGB;
	desc.bindFlags = BindFlag::SHADER_RESOURCE;
	desc.layout = ResourceState::SHADER_RESOURCE;

	// ScratchImage の並び
	std::vector<SubresourceData> subresources(mipImages.GetImageCount());
	for (size_t i = 0; i < subresources.size(); i++) {
		const auto &image = mipImages.GetImages()[i];
		subresources[i].dataPtr = image.pixels;
		subresources[i].rowPitch = static_cast<uint32_t>(image.rowPitch);
		subresources[i].slicePitch = static_cast<uint32_t>(image.slicePitch);
	}

	Entry entry{};
	entry.name = fileName;
	if (!GetDevice()->CreateTexture(&desc, subresources.data(), &entry.texture)) {
		FatalError("テクスチャの作成に失敗しました: " + std::string(fileName));
	}
	GetDevice()->SetName(&entry.texture, fileName);

	textures_.push_back(std::move(entry));
	return static_cast<uint32_t>(textures_.size() - 1);
}
