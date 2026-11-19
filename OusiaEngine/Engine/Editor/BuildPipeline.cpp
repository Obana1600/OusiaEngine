#include "BuildPipeline.h"
#include "Core/JsonFile.h"
#include "Core/PlayerSettings.h"
#include "Core/StringUtil.h"
#include "Diagnostics/Log.h"
#include "Editor/EditorBuildSettings.h"
#include "Platform/Paths.h"
#include "Scene/Scene.h"
#include "Scene/SceneManager.h"
#include "Scene/TagManager.h"
#include "Serialize/SceneSerializer.h"

#include <algorithm>
#include <filesystem>
#include <unordered_map>
#include <unordered_set>
#include <vector>
#include <optional>
#include <string>
#include <system_error>

#include <Windows.h>
#include <shellapi.h>

using namespace ou;


namespace {

struct PlayerSource {
	std::filesystem::path exe;
	std::filesystem::path folder;
};

bool IsFile(const std::filesystem::path &path) {
	std::error_code error;
	return std::filesystem::is_regular_file(path, error);
}

// 書き出しに使う Player（Release の exe）を探す
std::optional<PlayerSource> FindPlayer() {
	// 配ったエディタ: exe の隣の Player/Player.exe（Build/PackageEditor.ps1 が置く）
	const auto distributed = ExecutableDirectory() / L"Player";
	if (IsFile(distributed / L"Player.exe")) { return PlayerSource{distributed / L"Player.exe", distributed}; }

	// 開発中: リポジトリの Generated/Outputs/Release/<プロジェクト>/<プロジェクト>.exe
	const auto assets = FindAssetsFolder();
	if (!assets) { return std::nullopt; }
	const auto projectName = assets->parent_path().filename();
	const auto folder = std::filesystem::path(Utf8ToWide(SOURCE_DIR)).parent_path() / L"Generated/Outputs/Release" / projectName;
	const auto exe = folder / (projectName.wstring() + L".exe");
	if (IsFile(exe)) { return PlayerSource{exe, folder}; }
	return std::nullopt;
}

// ファイル名に使えない文字を _ にする
std::wstring MakeFileName(const std::string &productName) {
	auto name = Utf8ToWide(productName);
	for (auto &c : name) {
		if (wcschr(L"\\/:*?\"<>|", c)) { c = L'_'; }
	}
	return name.empty() ? std::wstring(L"Game") : name;
}

std::filesystem::path Normalize(const std::filesystem::path &path) {
	std::error_code error;
	auto normalized = std::filesystem::weakly_canonical(path, error);
	return error ? path.lexically_normal() : normalized;
}

// a が b と同じか、b の中にあるか
bool IsSameOrInside(const std::filesystem::path &a, const std::filesystem::path &b) {
	const auto relative = Normalize(a).lexically_relative(Normalize(b));
	return !relative.empty() && *relative.begin() != L"..";
}

bool CopyFolder(const std::filesystem::path &from, const std::filesystem::path &to) {
	std::error_code error;
	std::filesystem::copy(from, to, std::filesystem::copy_options::recursive | std::filesystem::copy_options::overwrite_existing, error);
	if (error) {
		Log::Error("コピーできませんでした: {} -> {} ({})", ToUtf8(from), ToUtf8(to), error.message());
		return false;
	}
	return true;
}

bool CopyOneFile(const std::filesystem::path &from, const std::filesystem::path &to) {
	std::error_code error;
	std::filesystem::copy_file(from, to, std::filesystem::copy_options::overwrite_existing, error);
	if (error) {
		Log::Error("コピーできませんでした: {} -> {} ({})", ToUtf8(from), ToUtf8(to), error.message());
		return false;
	}
	return true;
}

uintmax_t FolderSize(const std::filesystem::path &folder) {
	uintmax_t size = 0;
	std::error_code error;
	for (std::filesystem::recursive_directory_iterator it(folder, error), end; it != end; it.increment(error)) {
		if (error) { break; }
		if (it->is_regular_file(error)) { size += it->file_size(error); }
	}
	return size;
}

// ---- EditorOnly のタグの GameObject を外す ----

// 参照（{"id": 123}）の先を、消したものなら null にする
void ClearReferences(Json &json, const std::unordered_set<uint64_t> &removedIds) {
	if (json.is_object()) {
		if (json.size() == 1) {
			if (const auto it = json.find("id"); it != json.end() && it->is_number_unsigned() && removedIds.contains(it->get<uint64_t>())) {
				json = nullptr;
				return;
			}
		}
		for (auto &[key, value] : json.items()) {
			ClearReferences(value, removedIds);
		}
	} else if (json.is_array()) {
		for (auto &value : json) {
			ClearReferences(value, removedIds);
		}
	}
}

// root（シーン・プレハブの JSON）から、EditorOnly の GameObject と、その子を外す。外した数を返す
// keepsRoot なら、一番上の GameObject（プレハブの元）は外さない
size_t StripEditorOnly(Json &root, bool keepsRoot) {
	auto it = root.find("gameObjects");
	if (it == root.end() || !it->is_array()) { return 0; }
	auto &gameObjects = *it;

	// GameObject ごとに、Transform の $id と、親の Transform の $id（なければ 0）を引く
	const auto count = gameObjects.size();
	std::vector<uint64_t> transformIds(count, 0);
	std::vector<uint64_t> parentIds(count, 0);
	std::vector<bool> removes(count, false);
	std::unordered_map<uint64_t, size_t> indexByTransform;
	for (size_t i = 0; i < count; i++) {
		const auto &gameObject = gameObjects[i];
		if (!gameObject.is_object()) { continue; }

		if (const auto components = gameObject.find("components"); components != gameObject.end() && components->is_array()) {
			for (const auto &component : *components) {
				const auto parent = component.is_object() ? component.find("parent") : component.end();
				if (!component.is_object() || parent == component.end()) { continue; }

				if (const auto id = component.find("$id"); id != component.end() && id->is_number_unsigned()) {
					transformIds[i] = id->get<uint64_t>();
					indexByTransform[transformIds[i]] = i;
				}
				if (parent->is_object()) {
					if (const auto parentId = parent->find("id"); parentId != parent->end() && parentId->is_number_unsigned()) {
						parentIds[i] = parentId->get<uint64_t>();
					}
				}
				break;
			}
		}

		const auto tag = gameObject.find("tag");
		const auto isEditorOnly = tag != gameObject.end() && tag->is_string() && tag->get<std::string>() == TagManager::kEditorOnly;
		removes[i] = isEditorOnly && !(keepsRoot && parentIds[i] == 0);
	}

	// 外すものの子も外す（親が後ろに書かれていることもあるので、変わらなくなるまで）
	for (auto changed = true; changed;) {
		changed = false;
		for (size_t i = 0; i < count; i++) {
			if (removes[i] || parentIds[i] == 0) { continue; }
			const auto parent = indexByTransform.find(parentIds[i]);
			if (parent != indexByTransform.end() && removes[parent->second]) {
				removes[i] = true;
				changed = true;
			}
		}
	}

	// 外す GameObject とコンポーネントの $id
	std::unordered_set<uint64_t> removedIds;
	for (size_t i = 0; i < count; i++) {
		if (!removes[i]) { continue; }
		const auto &gameObject = gameObjects[i];
		if (const auto id = gameObject.find("$id"); id != gameObject.end() && id->is_number_unsigned()) { removedIds.insert(id->get<uint64_t>()); }
		if (const auto components = gameObject.find("components"); components != gameObject.end() && components->is_array()) {
			for (const auto &component : *components) {
				if (!component.is_object()) { continue; }
				if (const auto id = component.find("$id"); id != component.end() && id->is_number_unsigned()) { removedIds.insert(id->get<uint64_t>()); }
			}
		}
	}
	if (removedIds.empty()) { return 0; }

	Json kept = Json::array();
	size_t removedCount = 0;
	for (size_t i = 0; i < count; i++) {
		if (removes[i]) {
			removedCount++;
			continue;
		}
		kept.push_back(std::move(gameObjects[i]));
	}
	gameObjects = std::move(kept);

	// 残したものから、外したものへの参照を null にする（読み込むときに「参照先が見つかりません」と出ないように）
	ClearReferences(root, removedIds);
	return removedCount;
}

// 書き出した Assets の、シーンとプレハブから EditorOnly を外す（Unity と同じく、書き出したものだけを変える）
bool StripEditorOnlyInFolder(const std::filesystem::path &folder) {
	std::error_code error;
	size_t removedCount = 0;
	auto isSucceeded = true;
	std::vector<std::filesystem::path> objectsFolders;
	for (std::filesystem::recursive_directory_iterator it(folder, error), end; !error && it != end; it.increment(error)) {
		if (!it->is_regular_file(error)) { continue; }

		const auto extension = it->path().extension();
		const auto isScene = extension == L".scene";
		if (!isScene && extension != L".prefab") { continue; }

		// シーンは GameObject ごとのファイルを 1 つにまとめて書く（Player が読むファイルを減らすため）
		auto json = isScene ? SceneSerializer::LoadJson(it->path()) : JsonFile::Load(it->path());
		if (!json || !json->is_object()) { continue; }

		const auto removed = StripEditorOnly(*json, !isScene);
		if (removed == 0 && !isScene) { continue; }

		removedCount += removed;
		if (!JsonFile::Save(it->path(), *json)) {
			Log::Error("EditorOnly を外したものを書けませんでした: {}", ToUtf8(it->path()));
			isSucceeded = false;
		}
		if (isScene) { objectsFolders.push_back(SceneSerializer::ToObjectsFolder(it->path())); }
	}

	// 走査の途中で消すと iterator が壊れるので、あとでまとめて消す
	for (const auto &objectsFolder : objectsFolders) {
		std::filesystem::remove_all(objectsFolder, error);
	}

	if (removedCount > 0) { Log::Info("EditorOnly のタグの GameObject を {} 個外しました", removedCount); }
	return isSucceeded;
}

} // namespace



bool ou::BuildPipeline::BuildPlayer(const std::filesystem::path &location) {
	// ---- 書き出す前に確かめる ----
	const auto scenes = EditorBuildSettings::GetScenes();
	const auto hasScene = std::any_of(scenes.begin(), scenes.end(), [](const auto &scene) { return scene.enabled; });
	if (!hasScene) {
		Log::Error("Build Settings に有効なシーンがないため、書き出せません");
		return false;
	}

	const auto player = FindPlayer();
	if (!player) {
		Log::Error("書き出しに使う Player が見つかりません（配ったエディタなら Editor/Player/Player.exe、開発中なら Release のビルド）");
		return false;
	}

	const auto assets = FindAssetsFolder();
	const auto projectSettings = FindProjectSettingsFolder();
	const auto engineAssets = FindEngineAssetsFolder();
	if (!assets || !projectSettings || !engineAssets) {
		Log::Error("Assets・ProjectSettings・EngineAssets のどれかが見つからないため、書き出せません");
		return false;
	}

	// 書き出し先の Assets などは消してからコピーするので、プロジェクトそのもの・プロジェクトを含むフォルダ・Assets などの中は選べない
	// （プロジェクトの中でも Builds/ のような別のフォルダならよい。Unity と同じ）
	const auto project = assets->parent_path();
	const auto isInsideSource = IsSameOrInside(location, *assets) || IsSameOrInside(location, *projectSettings) ||
								IsSameOrInside(location, *engineAssets) || IsSameOrInside(location, project / L"UserSettings");
	if (IsSameOrInside(project, location) || isInsideSource) {
		Log::Error("その場所には書き出せません（プロジェクトのフォルダそのもの・それを含むフォルダ・Assets などの中は選べません）: {}", ToUtf8(location));
		return false;
	}

	if (const auto *scene = SceneManager::GetActiveScene(); scene && scene->IsDirty()) {
		Log::Warning("保存していないシーンの変更は、書き出したものに入りません: {}", scene->GetName());
	}

	// ---- 書き出す ----
	std::error_code error;
	std::filesystem::create_directories(location, error);
	for (const auto *folder : {L"Assets", L"EngineAssets", L"ProjectSettings", L"Runtime"}) {
		std::filesystem::remove_all(location / folder, error);
	}
	// DLL を exe の隣に置いていたころに書き出したものが残っていたら消す
	for (const auto *dll : {L"dxcompiler.dll", L"dxil.dll"}) {
		std::filesystem::remove(location / dll, error);
	}

	const auto exeName = MakeFileName(PlayerSettings::GetProductName()) + L".exe";
	auto isSucceeded = CopyOneFile(player->exe, location / exeName);
	std::filesystem::create_directories(location / L"Runtime", error);
	for (const auto *dll : {L"dxcompiler.dll", L"dxil.dll"}) {
		// 配ったエディタの Player/ には DLL を置かない（リポジトリを軽くするため）。そのときはエディタの Runtime/ のものを使う
		const auto fromPlayer = player->folder / L"Runtime" / dll;
		const auto source = IsFile(fromPlayer) ? fromPlayer : ExecutableDirectory() / L"Runtime" / dll;
		isSucceeded = isSucceeded && CopyOneFile(source, location / L"Runtime" / dll);
	}
	isSucceeded = isSucceeded && CopyFolder(*engineAssets, location / L"EngineAssets");
	isSucceeded = isSucceeded && CopyFolder(*assets, location / L"Assets");
	isSucceeded = isSucceeded && StripEditorOnlyInFolder(location / L"Assets");
	isSucceeded = isSucceeded && CopyFolder(*projectSettings, location / L"ProjectSettings");

	if (!isSucceeded) {
		Log::Error("書き出しに失敗しました: {}", ToUtf8(location));
		return false;
	}

	Log::Info("書き出しました: {} ({:.1f} MB)", ToUtf8(location / exeName), static_cast<double>(FolderSize(location)) / (1024.0 * 1024.0));

	// Unity と同じく、書き出したフォルダを開く
	ShellExecuteW(nullptr, L"open", location.c_str(), nullptr, nullptr, SW_SHOWNORMAL);
	return true;
}
