#pragma once

#include <filesystem>
#include <optional>
#include <span>
#include <stdexcept>
#include <string_view>

#include <nlohmann/json.hpp>



namespace ou {

// エイリアス
using Json = nlohmann::ordered_json;


class JsonFile final {
public: // 静的メンバ関数

	JsonFile() = delete;

	static std::optional<Json> Load(const std::filesystem::path &path);

	static bool Save(const std::filesystem::path &path, const Json &json);

	static void WarnUnknownKeys(const Json &object, std::span<const std::string_view> knownKeys, std::string_view source);

	class FormatError: public std::runtime_error {
	public: // コンストラクタ
		using std::runtime_error::runtime_error;
	};
};

} // namespace ou
