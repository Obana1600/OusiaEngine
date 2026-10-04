#include "Paths.h"

#include <ShlObj.h>
#include <Windows.h>
#include <algorithm>
#include <vector>

#pragma comment(lib, "Shell32.lib")


std::wstring ou::Utf8ToWide(std::string_view utf8) {
	if (utf8.empty()) {
		return {};
	}

	// 長さを測る
	auto length = MultiByteToWideChar(CP_UTF8, MB_ERR_INVALID_CHARS, utf8.data(), static_cast<int>(utf8.size()), nullptr, 0);
	if (length == 0) {
		return {};
	}

	// 実際に変換する
	std::wstring result(static_cast<size_t>(length), L'\0');
	MultiByteToWideChar(CP_UTF8, MB_ERR_INVALID_CHARS, utf8.data(), static_cast<int>(utf8.size()), result.data(), length);

	return result;
}


std::string ou::WideToUtf8(std::wstring_view wide) {
	if (wide.empty()) {
		return {};
	}

	// 長さを測る
	auto length = WideCharToMultiByte(CP_UTF8, 0, wide.data(), static_cast<int>(wide.size()), nullptr, 0, nullptr, nullptr);
	if (length == 0) {
		return {};
	}

	// 実際に変換する
	std::string result(static_cast<size_t>(length), '\0');
	WideCharToMultiByte(CP_UTF8, 0, wide.data(), static_cast<int>(wide.size()), result.data(), length, nullptr, nullptr);

	return result;
}


std::string ou::ToUtf8(const std::filesystem::path &path) {
	std::u8string u8 = path.u8string();
	return std::string(u8.begin(), u8.end());
}


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


std::optional<std::filesystem::path> ou::FindAssetsFolder() {
	const auto exeDir = ExecutableDirectory();

#ifdef WITH_EDITOR
	for (auto dir = exeDir;; dir = dir.parent_path()) {
		auto candidate = dir / L"Game" / L"Assets";
		if (std::filesystem::is_directory(candidate)) {
			return candidate;
		}
		if (dir == dir.root_path()) {
			return std::nullopt;
		}
	}
#else
	auto candidate = exeDir / L"Assets";
	if (std::filesystem::is_directory(candidate)) {
		return candidate;
	}
	return std::nullopt;
#endif
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
