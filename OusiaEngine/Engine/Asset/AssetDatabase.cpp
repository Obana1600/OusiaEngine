#include "AssetDatabase.h"
#include "Asset/AssetImporter.h"
#include "Asset/Importer/AudioImporter.h"
#include "Asset/Importer/FontImporter.h"
#include "Asset/Importer/MaterialImporter.h"
#include "Asset/Importer/ModelImporter.h"
#include "Asset/Importer/MonoScriptImporter.h"
#include "Asset/Importer/PhysicsMaterialImporter.h"
#include "Asset/Importer/PrefabImporter.h"
#include "Asset/Importer/ShaderImporter.h"
#include "Asset/Importer/ShaderModuleImporter.h"
#include "Asset/Importer/TextureImporter.h"
#include "Core/JsonFile.h"
#include "Core/StringUtil.h"
#include "Diagnostics/Log.h"
#include "Platform/Paths.h"
#include "Serialize/SceneSerializer.h"

#include <algorithm>
#include <cwctype>
#include <filesystem>
#include <functional>
#include <optional>
#include <string>
#include <system_error>
#include <unordered_map>
#include <utility>
#include <vector>

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
		{L".ttf", &MakeImporter<FontImporter>},
		{L".otf", &MakeImporter<FontImporter>},
		{L".physicsmaterial", &MakeImporter<PhysicsMaterialImporter>},
		{L".as", &MakeImporter<MonoScriptImporter>},
		{L".prefab", &MakeImporter<PrefabImporter>},
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

// GUID とパスのテーブル
struct GuidTable {
	std::unordered_map<Guid, std::string> pathsByGuid;
	std::unordered_map<std::string, Guid> guidsByPath;
};

// グローバルなインスタンス
GuidTable &Guids() {
	static GuidTable table;
	return table;
}

// .meta を作ったり書き直したりしてよいか
bool CanWriteMeta() {
#ifdef WITH_EDITOR
	return true;
#else
	return false;
#endif
}

// .meta かどうか
bool IsMetaFile(const std::filesystem::path &path) {
	auto extension = path.extension().wstring();
	for (auto &c : extension) {
		c = static_cast<wchar_t>(std::towlower(c));
	}
	return extension == L".meta";
}

// . で始まるものかどうか
bool IsHidden(const std::filesystem::path &path) {
	return path.filename().wstring().starts_with(L'.');
}

// json から GUID を読む
Guid ReadGuid(const Json &meta) {
	const auto it = meta.find("guid");
	if (it == meta.end() || !it->is_string()) { return Guid::Invalid; }

	return Guid::Parse(it->get<std::string>()).value_or(Guid::Invalid);
}

// GUID を入れて .meta を書く
bool WriteMeta(const std::filesystem::path &metaPath, Json meta, const Guid &guid) {
	meta["guid"] = guid.ToString();
	return JsonFile::Save(metaPath, meta);
}

// folder の下のファイルを table に足す
void ScanFolder(std::string_view root, const std::filesystem::path &folder, GuidTable &table) {
	std::vector<std::filesystem::path> assetFiles;
	std::vector<std::filesystem::path> metaFiles;

	std::error_code ec;
	for (std::filesystem::recursive_directory_iterator it(folder, ec), end; !ec && it != end; it.increment(ec)) {
		const auto &path = it->path();

		if (it->is_directory(ec)) {
			// シーンの GameObject のファイルは .scene の一部なので、アセットにしない（.meta も付けない）
			if (IsHidden(path) || SceneSerializer::IsObjectsFolder(path)) { it.disable_recursion_pending(); }
			continue;
		}
		if (!it->is_regular_file(ec) || IsHidden(path)) { continue; }

		(IsMetaFile(path) ? metaFiles : assetFiles).push_back(path);
	}
	if (ec) {
		Log::Warning("フォルダの走査が途中で止まりました: {}", ToUtf8(folder));
	}

	std::sort(assetFiles.begin(), assetFiles.end());

	for (const auto &fullPath : assetFiles) {
		const auto assetPath = std::string(root) + "/" + WideToUtf8(fullPath.lexically_relative(folder).generic_wstring());

		auto metaPath = fullPath;
		metaPath += L".meta";

		auto meta = Json::object();
		auto guid = Guid::Invalid;
		if (std::filesystem::exists(metaPath, ec)) {
			if (auto loaded = JsonFile::Load(metaPath); loaded && loaded->is_object()) {
				meta = std::move(*loaded);
				guid = ReadGuid(meta);
			}
			if (!guid.IsValid()) {
				Log::Warning(".meta の guid の読み込みに失敗しました: {}.meta", assetPath);
			}
		}

		if (const auto it = table.pathsByGuid.find(guid); it != table.pathsByGuid.end()) {
			Log::Warning("GUID が重複しています: {} と {}", it->second, assetPath);
			guid = Guid::Invalid;
		}

		if (!guid.IsValid()) {
			if (!CanWriteMeta()) {
				Log::Error("GUID がないため、シーンから参照できません: {}", assetPath);
				continue;
			}

			guid = Guid::Generate();
			if (!WriteMeta(metaPath, std::move(meta), guid)) {
				Log::Error(".meta の書き込みに失敗しました: {}.meta", assetPath);
				continue;
			}
			Log::Info(".meta を作成しました: {}.meta", assetPath);
		}

		table.pathsByGuid.emplace(guid, assetPath);
		table.guidsByPath.emplace(assetPath, guid);
	}

	for (const auto &metaPath : metaFiles) {
		auto assetFullPath = metaPath;
		assetFullPath.replace_extension();
		if (!std::filesystem::is_regular_file(assetFullPath, ec)) {
			Log::Warning("相手のファイルがない .meta です: {}", ToUtf8(metaPath));
		}
	}
}

// オブジェクトの出どころ
struct AssetOrigin {
	std::string assetPath;
	std::string subAsset;
};

// グローバルなキー
std::unordered_map<uint32_t, AssetOrigin> &Origins() {
	static std::unordered_map<uint32_t, AssetOrigin> origins;
	return origins;
}

// インポートして、できたオブジェクトの出どころを覚える
std::shared_ptr<Object> ImportEntry(const std::string &key, AssetEntry &entry) {
	entry.importer->ClearSubAssets();

	auto asset = entry.importer->Import();
	if (!asset) { return nullptr; }

	auto &origins = Origins();
	origins[asset->GetInstanceId()] = {key, {}};
	for (const auto &sub : entry.importer->GetSubAssets()) {
		if (const auto object = sub.object.lock()) {
			origins[object->GetInstanceId()] = {key, sub.name};
		}
	}

	return asset;
}


// ASCII だけ小文字にする
std::string ToLowerAscii(std::string_view text) {
	std::string result(text);
	for (auto &c : result) {
		if (c >= 'A' && c <= 'Z') { c = static_cast<char>(c - 'A' + 'a'); }
	}
	return result;
}

// 拡張子から、FindAssets の t: に使える型の名前
bool MatchesAssetType(const std::filesystem::path &path, std::string_view typeName) {
	struct TypeEntry {
		std::string_view extension;
		std::string_view typeName;
	};
	static constexpr TypeEntry kTypes[] = {
		{".scene", "scene"},
		{".png", "texture2d"},
		{".png", "texture"},
		{".jpg", "texture2d"},
		{".jpg", "texture"},
		{".jpeg", "texture2d"},
		{".jpeg", "texture"},
		{".mat", "material"},
		{".shader", "shader"},
		{".wav", "audioclip"},
		{".mp3", "audioclip"},
		{".ttf", "font"},
		{".ttf", "fontasset"},
		{".otf", "font"},
		{".otf", "fontasset"},
		{".physicsmaterial", "physicsmaterial"},
		{".gltf", "model"},
		{".gltf", "gameobject"},
		{".glb", "model"},
		{".glb", "gameobject"},
		{".obj", "model"},
		{".obj", "gameobject"},
		{".as", "monoscript"},
		{".as", "script"},
		{".prefab", "prefab"},
		{".prefab", "gameobject"},
	};

	const auto extension = ToLowerAscii(ToUtf8(path.extension()));
	for (const auto &entry : kTypes) {
		if (entry.extension == extension && entry.typeName == typeName) { return true; }
	}
	return false;
}

} // namespace



std::shared_ptr<Object> ou::AssetDatabase::LoadMainAssetAtPath(std::string_view assetPath) {
	const auto key = NormalizeAssetPath(assetPath);
	auto *entry = FindOrCreateEntry(key);
	if (!entry) { return nullptr; }

	// アセットがなければ読み込む
	if (!entry->asset) {
		entry->asset = ImportEntry(key, *entry);
	}
	return entry->asset;
}


AssetImporter *ou::AssetDatabase::GetImporterAtPath(std::string_view assetPath) {
	const auto key = NormalizeAssetPath(assetPath);
	auto *entry = FindOrCreateEntry(key);

	return entry ? entry->importer.get() : nullptr;
}


bool ou::AssetDatabase::CanImport(std::string_view assetPath) {
	const auto key = NormalizeAssetPath(assetPath);
	if (Entries().contains(key)) { return true; }

	const auto fullPath = ToFullPath(key);
	std::error_code error;
	if (!fullPath || std::filesystem::is_directory(*fullPath, error)) { return false; }

	// 拡張子だけを見る（インポーターは作ってすぐ捨てる）
	return CreateImporter(key, *fullPath) != nullptr;
}


void ou::AssetDatabase::ImportAsset(std::string_view assetPath) {
	const auto key = NormalizeAssetPath(assetPath);
	auto *entry = FindOrCreateEntry(key);
	if (!entry) { return; }

	auto asset = ImportEntry(key, *entry);
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
	Origins().clear();
}


void ou::AssetDatabase::Refresh() {
	auto &table = Guids();
	table = {};

	if (const auto folder = FindEngineAssetsFolder()) {
		ScanFolder("EngineAssets", *folder, table);
	}
	if (const auto folder = FindAssetsFolder()) {
		ScanFolder("Assets", *folder, table);
	}
}


Guid ou::AssetDatabase::AssetPathToGuid(std::string_view assetPath) {
	const auto &guids = Guids().guidsByPath;
	const auto it = guids.find(NormalizeAssetPath(assetPath));
	return it != guids.end() ? it->second : Guid::Invalid;
}


std::string ou::AssetDatabase::GuidToAssetPath(const Guid &guid) {
	const auto &paths = Guids().pathsByGuid;
	const auto it = paths.find(guid);
	return it != paths.end() ? it->second : std::string();
}


std::optional<std::filesystem::path> ou::AssetDatabase::AssetPathToFullPath(std::string_view assetPath) {
	return ToFullPath(NormalizeAssetPath(assetPath));
}


std::optional<ou::AssetDatabase::AssetReference> ou::AssetDatabase::FindAssetReference(const Object *asset) {
	if (!asset) { return std::nullopt; }

	const auto &origins = Origins();
	const auto it = origins.find(asset->GetInstanceId());
	if (it == origins.end()) { return std::nullopt; }

	const auto guid = AssetPathToGuid(it->second.assetPath);
	if (!guid.IsValid()) { return std::nullopt; }

	return AssetReference{guid, it->second.subAsset};
}


std::shared_ptr<Object> ou::AssetDatabase::LoadAsset(const Guid &guid, std::string_view subAsset) {
	const auto assetPath = GuidToAssetPath(guid);
	if (assetPath.empty()) {
		Log::Warning("この GUID のアセットが見つかりません: {}", guid.ToString());
		return nullptr;
	}

	auto main = LoadMainAssetAtPath(assetPath);
	if (!main || subAsset.empty()) { return main; }

	if (const auto *importer = GetImporterAtPath(assetPath)) {
		for (const auto &sub : importer->GetSubAssets()) {
			if (sub.name != subAsset) { continue; }
			if (auto object = sub.object.lock()) { return object; }
		}
	}

	Log::Warning("サブアセットが見つかりません: {}: {}", assetPath, subAsset);
	return nullptr;
}


std::vector<Guid> ou::AssetDatabase::FindAssets(std::string_view filter) {
	std::vector<std::string> typeNames;
	std::vector<std::string> nameParts;
	size_t begin = 0;
	while (begin < filter.size()) {
		const auto end = std::min(filter.find(' ', begin), filter.size());
		const auto word = ToLowerAscii(filter.substr(begin, end - begin));
		if (word.starts_with("t:")) { // TODO: "t:" 以外のフィルタにも対応する
			typeNames.push_back(word.substr(2));
		} else if (!word.empty()) {
			nameParts.push_back(word);
		}
		begin = end + 1;
	}

	std::vector<std::pair<std::string, Guid>> found;
	for (const auto &[guid, assetPath] : Guids().pathsByGuid) {
		const auto path = std::filesystem::path(Utf8ToWide(assetPath));
		const auto name = ToLowerAscii(ToUtf8(path.stem()));

		const auto matchesType = typeNames.empty() ||
								 std::any_of(typeNames.begin(), typeNames.end(),
											 [&](const std::string &type) { return MatchesAssetType(path, type); });
		const auto matchesName = std::all_of(nameParts.begin(), nameParts.end(),
											 [&](const std::string &part) { return name.find(part) != std::string::npos; });

		if (matchesType && matchesName) {
			found.emplace_back(assetPath, guid);
		}
	}
	std::sort(found.begin(), found.end(), [](const auto &a, const auto &b) { return a.first < b.first; });

	std::vector<Guid> guids;
	guids.reserve(found.size());
	for (const auto &[path, guid] : found) {
		guids.push_back(guid);
	}
	return guids;
}


std::string ou::AssetDatabase::MoveAsset(std::string_view oldPath, std::string_view newPath) {
	const auto from = NormalizeAssetPath(oldPath);
	const auto to = NormalizeAssetPath(newPath);

	// path が folder 自身か、その中か
	const auto isSameOrInside = [](const std::string &path, const std::string &folder) {
		return path == folder || path.starts_with(folder + "/");
	};

	if (!from.starts_with("Assets/") || !to.starts_with("Assets/")) { return "動かせるのは Assets の中だけです"; }
	if (from == to) { return {}; }
	if (isSameOrInside(to, from)) { return "フォルダを自分の中へは動かせません"; }

	const auto fromFull = ToFullPath(from);
	const auto toFull = ToFullPath(to);
	if (!fromFull || !toFull) { return "パスが正しくありません"; }

	std::error_code error;
	if (!std::filesystem::exists(*fromFull, error)) { return "元のファイルが見つかりません: " + from; }
	// 大文字と小文字だけを変えるときは、Windows では同じものとして見つかるので、それは許す
	if (std::filesystem::exists(*toFull, error) && !std::filesystem::equivalent(*fromFull, *toFull, error)) {
		return "同じ名前のものがもうあります: " + to;
	}
	if (!std::filesystem::is_directory(toFull->parent_path(), error)) { return "動かす先のフォルダがありません"; }

	const auto isFolder = std::filesystem::is_directory(*fromFull, error);
	std::filesystem::rename(*fromFull, *toFull, error);
	if (error) { return "動かせませんでした: " + error.message(); }

	// ファイルは .meta も一緒に（フォルダは .meta を持たない）
	if (!isFolder) {
		auto fromMeta = *fromFull;
		fromMeta += L".meta";
		auto toMeta = *toFull;
		toMeta += L".meta";
		if (std::filesystem::exists(fromMeta, error)) {
			std::filesystem::rename(fromMeta, toMeta, error);
			if (error) { Log::Error(".meta を動かせませんでした (GUID が変わります): {}.meta", from); }
		}

		// シーンは GameObject のファイルのフォルダも一緒に（.scene はもう動かしたので、IsObjectsFolder では確かめられない）
		const auto fromObjects = SceneSerializer::ToObjectsFolder(*fromFull);
		if (_wcsicmp(fromFull->extension().c_str(), L".scene") == 0 && std::filesystem::is_directory(fromObjects, error)) {
			std::filesystem::rename(fromObjects, SceneSerializer::ToObjectsFolder(*toFull), error);
			if (error) { Log::Error("シーンの GameObject のフォルダを動かせませんでした: {}", ToUtf8(fromObjects)); }
		}
	}

	const auto movePath = [&](const std::string &path) { return to + path.substr(from.size()); };

	// 読み込み済みのものを、新しいパスで引けるようにする（読み直すと、別のオブジェクトになって参照が外れるため）
	auto &entries = Entries();
	std::vector<std::string> movedKeys;
	for (const auto &[key, entry] : entries) {
		if (isSameOrInside(key, from)) { movedKeys.push_back(key); }
	}
	for (const auto &key : movedKeys) {
		auto node = entries.extract(key);
		const auto newKey = movePath(key);
		if (const auto newFull = ToFullPath(newKey); newFull && node.mapped().importer) {
			node.mapped().importer->SetPath(newKey, *newFull);
		}
		// Unity と同じく、アセットの名前はファイル名（拡張子なし）
		if (node.mapped().asset) {
			node.mapped().asset->SetName(WideToUtf8(std::filesystem::path(Utf8ToWide(newKey)).stem().wstring()));
		}
		node.key() = newKey;
		entries.insert(std::move(node));
	}

	for (auto &[instanceId, origin] : Origins()) {
		if (isSameOrInside(origin.assetPath, from)) { origin.assetPath = movePath(origin.assetPath); }
	}

	// GUID の表を作り直す（.meta ごと動かしたので、同じ GUID が新しいパスを指す）
	Refresh();

	Log::Info("{} を {} にしました", from, to);
	return {};
}
