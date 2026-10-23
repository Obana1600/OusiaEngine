#include "FontImporter.h"
#include "Core/StringUtil.h"
#include "Diagnostics/Log.h"
#include "UI/Asset/FontAsset.h"

#include <fstream>
#include <optional>
#include <utility>
#include <vector>

using namespace ou;


namespace {

std::optional<std::vector<uint8_t>> ReadAllBytes(const std::filesystem::path &path) {
	std::ifstream file(path, std::ios::binary | std::ios::ate);
	if (!file) { return std::nullopt; }

	// ファイル末尾でファイルサイズを調べる
	const auto size = static_cast<std::streamsize>(file.tellg());
	if (size <= 0) { return std::nullopt; }
	std::vector<uint8_t> data(static_cast<size_t>(size));

	// 先頭に戻りデータを取り出す
	file.seekg(0);
	if (!file.read(reinterpret_cast<char *>(data.data()), size)) {
		return std::nullopt;
	}

	return data;
}

} // namespace



ou::FontImporter::FontImporter(std::string assetPath, std::filesystem::path fullPath)
	: AssetImporter(std::move(assetPath), std::move(fullPath)) {}


std::shared_ptr<Object> ou::FontImporter::Import() {
	return ImportFontAsset();
}


std::shared_ptr<FontAsset> ou::FontImporter::ImportFontAsset() {
	// フォントファイルの読み込み
	auto data = ReadAllBytes(GetFullPath());
	if (!data) {
		Log::Error("フォントファイルの読み込みに失敗しました: {}", ToUtf8(GetFullPath()));
		return nullptr;
	}

	// フォントアセットの作成
	auto font = FontAsset::Create(std::move(*data), samplingPointSize_, padding_, atlasSize_);
	if (!font) {
		Log::Error("フォントアセットの作成に失敗しました: {}", ToUtf8(GetFullPath()));
		return nullptr;
	}
	font->SetName(GetName());

	return font;
}


void ou::FontImporter::SetSamplingPointSize(uint32_t samplingPointSize) {
	samplingPointSize_ = samplingPointSize;
}


void ou::FontImporter::SetPadding(uint32_t padding) {
	padding_ = padding;
}


void ou::FontImporter::SetAtlasSize(uint32_t atlasSize) {
	atlasSize_ = atlasSize;
}
