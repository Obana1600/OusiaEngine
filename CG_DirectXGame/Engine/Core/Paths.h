#pragma once

#include <filesystem>
#include <optional>
#include <string>
#include <string_view>


namespace ou {

// UTF-8 -> UTF-16
std::wstring Utf8ToWide(std::string_view utf8);

// UTF-16 -> UTF-8
std::string WideToUtf8(std::wstring_view wide);

// filePath -> UTF-8
std::string ToUtf8(const std::filesystem::path &path);

// 実行している .exe の親ディレクトリのパスを取得する
std::filesystem::path ExecutableDirectory();

// Assets フォルダのパスを取得する
std::optional<std::filesystem::path> FindAssetsFolder();

// EngineAssets フォルダ（エンジンが使うシェーダなど）のパスを取得する
std::optional<std::filesystem::path> FindEngineAssetsFolder();

// UserSettings フォルダのパスを取得する
std::optional<std::filesystem::path> FindUserSettingsFolder();

// ローカルの AppData ディレクトリを取得する
std::filesystem::path LocalAppDataDirectory();

// ログとダンプを置くディレクトリのパスを取得する
std::filesystem::path LogDirectory(std::string_view gameName);

// keepCount 以下のファイルを削除する
void RemoveOldFiles(const std::filesystem::path &directory, const wchar_t *extension, int keepCount);

} // namespace ou
