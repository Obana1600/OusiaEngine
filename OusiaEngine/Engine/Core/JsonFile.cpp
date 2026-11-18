#include "JsonFile.h"
#include "Core/StringUtil.h"
#include "Diagnostics/Log.h"

#include <algorithm>
#include <fstream>

using namespace ou;


std::optional<Json> ou::JsonFile::Load(const std::filesystem::path &path) {
	std::ifstream file(path, std::ios::binary);
	if (!file) {
		Log::Error("ファイルの読み込みに失敗しました: {}", ToUtf8(path));
		return std::nullopt;
	}

	try {
		return Json::parse(file, nullptr, true, true);
	} catch (const Json::parse_error &e) {
		Log::Error("json の書き方が間違っています: {}\n{}", ToUtf8(path), e.what());
		return std::nullopt;
	}
}


bool ou::JsonFile::Save(const std::filesystem::path &path, const Json &json) {
	std::error_code error;
	std::filesystem::create_directories(path.parent_path(), error);

	std::ofstream file(path, std::ios::binary);
	if (!file) {
		Log::Error("ファイルの書き込みに失敗しました: {}", ToUtf8(path));
		return false;
	}

	file << json.dump(1, '\t') << '\n';
	return static_cast<bool>(file);
}


void ou::JsonFile::WarnUnknownKeys(const Json &object, std::span<const std::string_view> knownKeys, std::string_view source) {
	for (const auto &[key, value] : object.items()) {
		if (std::find(knownKeys.begin(), knownKeys.end(), key) == knownKeys.end()) {
			Log::Warning("無効なキーです: {}: \"{}\"", source, key);
		}
	}
}
