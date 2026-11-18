#include "BuildPipeline.h"
#include "Core/PlayerSettings.h"
#include "Core/StringUtil.h"
#include "Diagnostics/Log.h"
#include "Editor/EditorBuildSettings.h"
#include "Platform/Paths.h"
#include "Scene/Scene.h"
#include "Scene/SceneManager.h"

#include <algorithm>
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
