#include "Paths.h"
#include "Core/StringUtil.h"

#include <ShlObj.h>
#include <Windows.h>
#include <algorithm>
#include <vector>

#pragma comment(lib, "Shell32.lib")


namespace {

// プロジェクトのフォルダパス
std::filesystem::path &ProjectDirectory() {
	static std::filesystem::path directory;
	return directory;
}

// エディタ版はソースフォルダの中を直接、それ以外は exe の隣を見る
std::optional<std::filesystem::path> FindFolder(const std::filesystem::path &editorFolder, const wchar_t *shippingRelative) {
#ifdef WITH_EDITOR
	(void)shippingRelative;
	auto candidate = editorFolder;
#else
	(void)editorFolder;
	auto candidate = ou::ExecutableDirectory() / shippingRelative;
#endif

	if (std::filesystem::is_directory(candidate)) {
		return candidate;
	}
	return std::nullopt;
}

} // namespace



std::filesystem::path ou::ExecutableDirectory() {
	std::vector<wchar_t> buffer(MAX_PATH);

	while (true) {
		// パスとその長さを取得
		auto length = GetModuleFileNameW(nullptr, buffer.data(), static_cast<DWORD>(buffer.size()));

		if (length < buffer.size()) {
			std::wstring_view view(buffer.data(), length);

			// 実行ファイルの親ディレクトリを返す
			return std::filesystem::path(view).parent_path();
		}

		// サイズが足りなかったら拡張
		buffer.resize(buffer.size() * 2);
	}
}


void ou::SetProjectDirectory(const std::filesystem::path &directory) {
	ProjectDirectory() = directory;
}


std::optional<std::filesystem::path> ou::FindAssetsFolder() {
	return FindFolder(ProjectDirectory() / L"Assets", L"Assets");
}


std::optional<std::filesystem::path> ou::FindEngineAssetsFolder() {
	return FindFolder(std::filesystem::path(Utf8ToWide(SOURCE_DIR)) / L"Engine/EngineAssets", L"EngineAssets");
}


std::optional<std::filesystem::path> ou::FindUserSettingsFolder() {
	const auto assets = FindAssetsFolder();
	if (!assets) {
		return std::nullopt;
	}

	auto folder = assets->parent_path() / L"UserSettings";
	std::error_code ec;
	std::filesystem::create_directories(folder, ec);
	return folder;
}


std::filesystem::path ou::LocalAppDataDirectory() {
	PWSTR raw = nullptr;
	std::filesystem::path result;

	auto hr = SHGetKnownFolderPath(FOLDERID_LocalAppData, 0, nullptr, &raw);
	if (SUCCEEDED(hr)) {
		result = raw;
	}

	CoTaskMemFree(raw);
	return result;
}


std::filesystem::path ou::LogDirectory(std::string_view gameName) {
#ifdef WITH_EDITOR
	(void)gameName;
	return ExecutableDirectory() / L"Logs";
#else

	const auto base = LocalAppDataDirectory();
	if (base.empty()) {
		return ExecutableDirectory() / L"Logs";
	}

	return base / L"OusiaEngine" / Utf8ToWide(gameName) / L"Logs";

#endif
}


void ou::RemoveOldFiles(const std::filesystem::path &directory, const wchar_t *extension, int keepCount) {
	std::error_code ec;

	std::vector<std::filesystem::path> files;
	for (const auto &entry : std::filesystem::directory_iterator(directory, ec)) {
		if (entry.is_regular_file(ec) && entry.path().extension() == extension) {
			files.push_back(entry.path());
		}
	}

	std::ranges::sort(files);

	const auto removeCount = static_cast<int>(files.size()) - (keepCount - 1);
	for (int i = 0; i < removeCount; i++) {
		std::filesystem::remove(files[static_cast<size_t>(i)], ec);
	}
}
