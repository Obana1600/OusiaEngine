#include "AssetDatabase.h"
#include "Asset/AssetImporter.h"
#include "Asset/Importer/AudioImporter.h"
#include "Asset/Importer/MaterialImporter.h"
#include "Asset/Importer/ModelImporter.h"
#include "Asset/Importer/ShaderImporter.h"
#include "Asset/Importer/ShaderModuleImporter.h"
#include "Asset/Importer/TextureImporter.h"
#include "Core/Log.h"
#include "Core/Paths.h"

#include <cwctype>
#include <filesystem>
#include <functional>
#include <optional>
#include <string>
#include <unordered_map>
#include <utility>

using namespace ou;


namespace {

// キャッシュの 1 件
struct AssetEntry {
	std::unique_ptr<AssetImporter> importer;
	std::shared_ptr<Object> asset;
};

// グローバルなキャッシュ
std::unordered_map<std::string, AssetEntry> &Entries() {
	static std::unordered_map<std::string, AssetEntry> entries;
	return entries;
}

// パスの正規化
std::string NormalizeAssetPath(std::string_view assetPath) {
	const auto normalized = std::filesystem::path(Utf8ToWide(assetPath)).lexically_normal();
	return WideToUtf8(normalized.generic_wstring());
}

// 先頭の "Assets" / "EngineAssets" を実際のパスに置き換える
std::optional<std::filesystem::path> ToFullPath(std::string_view assetPath) {
	const auto slash = assetPath.find('/');
	if (slash == std::string_view::npos) { return std::nullopt; }

	const auto root = assetPath.substr(0, slash);
	const auto rest = assetPath.substr(slash + 1);

	std::optional<std::filesystem::path> folder;
	if (root == "Assets") {
		folder = FindAssetsFolder();
	} else if (root == "EngineAssets") {
		folder = FindEngineAssetsFolder();
	}
	if (!folder) { return std::nullopt; }

	return *folder / Utf8ToWide(rest);
}

// T へ派生させたインポーターを返す
template <typename T>
std::unique_ptr<AssetImporter> MakeImporter(std::string assetPath, std::filesystem::path fullPath) {
	return std::make_unique<T>(std::move(assetPath), std::move(fullPath));
}

// 拡張子からインポーターを作る
std::unique_ptr<AssetImporter> CreateImporter(const std::string &assetPath, const std::filesystem::path &fullPath) {
	// 拡張子とファクトリのテーブル
	struct ImporterEntry {
		std::wstring_view extension;
		std::function<std::unique_ptr<AssetImporter>(std::string, std::filesystem::path)> factory;
	};
	static const ImporterEntry kImporters[] = {
		{L".hlsl", &MakeImporter<ShaderModuleImporter>},
		{L".png", &MakeImporter<TextureImporter>},
		{L".jpg", &MakeImporter<TextureImporter>},
		{L".jpeg", &MakeImporter<TextureImporter>},
		{L".wav", &MakeImporter<AudioImporter>},
		{L".mp3", &MakeImporter<AudioImporter>},
		{L".shader", &MakeImporter<ShaderImporter>},
		{L".mat", &MakeImporter<MaterialImporter>},
		{L".obj", &MakeImporter<ModelImporter>},
		{L".gltf", &MakeImporter<ModelImporter>},
		{L".glb", &MakeImporter<ModelImporter>},
	};

	// 拡張子を小文字のワイド文字にそろえる
	auto extension = fullPath.extension().wstring();
	for (auto &c : extension) {
		c = static_cast<wchar_t>(std::towlower(c));
	}

	for (const auto &entry : kImporters) {
		if (extension == entry.extension) {
			return entry.factory(assetPath, fullPath);
		}
	}
	return nullptr;
}

// キャッシュを探し、なければインポーターだけ作りキャッシュする
AssetEntry *FindOrCreateEntry(const std::string &key) {
	auto &entries = Entries();
	if (const auto it = entries.find(key); it != entries.end()) {
		return &it->second;
	}

	const auto fullPath = ToFullPath(key);
	if (!fullPath) {
		Log::Error("アセットのパスは Assets/ か EngineAssets/ で始めてください: {}", key);
		return nullptr;
	}

	auto importer = CreateImporter(key, *fullPath);
	if (!importer) {
		Log::Error("この拡張子のアセットは対応していません: {}", key);
		return nullptr;
	}

	// インポーターだけ作る
	const auto [it, inserted] = entries.emplace(key, AssetEntry(std::move(importer), nullptr));
	return &it->second;
}

} // namespace



std::shared_ptr<Object> ou::AssetDatabase::LoadMainAssetAtPath(std::string_view assetPath) {
	const auto key = NormalizeAssetPath(assetPath);
	auto *entry = FindOrCreateEntry(key);
	if (!entry) { return nullptr; }

	// アセットがなければ読み込む
	if (!entry->asset) {
		entry->asset = entry->importer->Import();
	}
	return entry->asset;
}


AssetImporter *ou::AssetDatabase::GetImporterAtPath(std::string_view assetPath) {
	const auto key = NormalizeAssetPath(assetPath);
	auto *entry = FindOrCreateEntry(key);

	return entry ? entry->importer.get() : nullptr;
}


void ou::AssetDatabase::ImportAsset(std::string_view assetPath) {
	const auto key = NormalizeAssetPath(assetPath);
	auto *entry = FindOrCreateEntry(key);
	if (!entry) { return; }

	auto asset = entry->importer->Import();
	if (!asset) { return; }
	entry->asset = std::move(asset);
}


void ou::AssetDatabase::UnloadUnusedAssets() {
	for (auto &[key, entry] : Entries()) {
		if (entry.asset && entry.asset.use_count() == 1) {
			entry.asset.reset();
		}
	}
}


void ou::AssetDatabase::UnloadAllAssets() {
	Entries().clear();
}
