#include "AssetImporter.h"
#include "Asset/AssetDatabase.h"
#include "Core/JsonFile.h"
#include "Core/StringUtil.h"
#include "Diagnostics/Log.h"
#include "Serialize/JsonReader.h"
#include "Serialize/JsonWriter.h"

#include <algorithm>
#include <unordered_map>
#include <unordered_set>
#include <utility>

using namespace ou;


namespace {

constexpr const char *kImportSettingsKey = "importSettings";

std::filesystem::path MakeMetaPath(const std::filesystem::path &fullPath) {
	auto metaPath = fullPath;
	metaPath += L".meta";
	return metaPath;
}

} // namespace


const std::string &ou::AssetImporter::GetAssetPath() const {
	return assetPath_;
}


const std::filesystem::path &ou::AssetImporter::GetFullPath() const {
	return fullPath_;
}


void ou::AssetImporter::SetPath(std::string assetPath, std::filesystem::path fullPath) {
	assetPath_ = std::move(assetPath);
	fullPath_ = std::move(fullPath);
	SetName(ToUtf8(fullPath_.filename()));
}


std::string ou::AssetImporter::ResolveAssetPath(std::string_view relativePath) const {
	const auto slash = assetPath_.find_last_of('/');
	const auto folder = (slash == std::string::npos) ? std::string()
													 : assetPath_.substr(0, slash + 1);
	return folder + std::string(relativePath);
}


const std::vector<AssetImporter::SubAsset> &ou::AssetImporter::GetSubAssets() const {
	return subAssets_;
}


void ou::AssetImporter::ClearSubAssets() {
	subAssets_.clear();
}


#ifdef WITH_EDITOR
void ou::AssetImporter::SaveAndReimport() {
	const auto metaPath = MakeMetaPath(fullPath_);

	auto meta = Json::object();
	std::error_code error;
	if (std::filesystem::exists(metaPath, error)) {
		if (auto loaded = JsonFile::Load(metaPath); loaded && loaded->is_object()) {
			meta = std::move(*loaded);
		}
	}

	auto settings = Json::object();
	const std::unordered_set<const Object *> noSceneObjects;
	JsonWriter writer(settings, noSceneObjects);
	Serialize(writer);
	meta[kImportSettingsKey] = std::move(settings);

	if (!JsonFile::Save(metaPath, meta)) {
		Log::Error("インポートの設定を書けませんでした: {}.meta", assetPath_);
		return;
	}
	AssetDatabase::ImportAsset(assetPath_);
}
#endif


ou::AssetImporter::AssetImporter(std::string assetPath, std::filesystem::path fullPath)
	: assetPath_(std::move(assetPath)),
	  fullPath_(std::move(fullPath)) {
	SetName(ToUtf8(fullPath_.filename()));
}


void ou::AssetImporter::AddSubAsset(std::string_view typeName, const std::shared_ptr<Object> &object) {
	if (!object) { return; }

	const auto &objectName = object->GetName();
	const auto base = std::string(typeName) + "/" + (objectName.empty() ? std::string(typeName) : objectName);

	const auto isUsed = [this](const std::string &name) {
		return std::any_of(subAssets_.begin(), subAssets_.end(),
						   [&](const SubAsset &sub) { return sub.name == name; });
	};

	auto name = base;
	for (int suffix = 1; isUsed(name); suffix++) {
		name = base + "_" + std::to_string(suffix);
	}

	subAssets_.push_back({std::move(name), object});
}


void ou::AssetImporter::ReadImportSettings() {
	const auto metaPath = MakeMetaPath(fullPath_);

	std::error_code error;
	if (!std::filesystem::exists(metaPath, error)) { return; }

	const auto meta = JsonFile::Load(metaPath);
	if (!meta || !meta->is_object()) { return; }

	const auto it = meta->find(kImportSettingsKey);
	if (it == meta->end()) { return; }
	if (!it->is_object()) {
		Log::Warning(".meta の {} がオブジェクトではありません: {}.meta", kImportSettingsKey, assetPath_);
		return;
	}

	const std::unordered_map<uint64_t, Object *> noSceneObjects;
	JsonReader reader(*it, noSceneObjects);
	Serialize(reader);
}
