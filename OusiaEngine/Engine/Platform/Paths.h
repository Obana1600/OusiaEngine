#pragma once

#include <filesystem>
#include <optional>
#include <string_view>


namespace ou {

// 実行している .exe の親ディレクトリのパスを取得する
std::filesystem::path ExecutableDirectory();

// プロジェクトのフォルダを設定する
void SetProjectDirectory(const std::filesystem::path &directory);

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
