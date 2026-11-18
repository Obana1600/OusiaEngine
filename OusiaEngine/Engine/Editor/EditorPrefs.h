#pragma once

#include <string>
#include <string_view>


namespace ou {

// Unity の EditorPrefs。PC ごとの設定（どのプロジェクトでも同じ）
// %LOCALAPPDATA%/OusiaEngine/EditorPrefs.json に保存する
class EditorPrefs final {
public: // 静的メンバ関数

	EditorPrefs() = delete;

	static std::string GetString(std::string_view key, std::string_view defaultValue = {});
	static void SetString(std::string_view key, std::string_view value);

	static int GetInt(std::string_view key, int defaultValue = 0);
	static void SetInt(std::string_view key, int value);

	static bool GetBool(std::string_view key, bool defaultValue = false);
	static void SetBool(std::string_view key, bool value);

	static bool HasKey(std::string_view key);
	static void DeleteKey(std::string_view key);
};

} // namespace ou
