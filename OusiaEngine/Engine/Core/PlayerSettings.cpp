#include "PlayerSettings.h"
#include "Core/JsonFile.h"
#include "Platform/Paths.h"

#include <algorithm>
#include <filesystem>
#include <optional>
#include <system_error>

#include <magic_enum/magic_enum.hpp>

using namespace ou;


namespace {

// 設定を書き込むファイル名
constexpr const wchar_t *kSettingsFileName = L"PlayerSettings.json";

// 既定のアプリ名
constexpr const char *kDefaultProductName = "OusiaEngine_Project";

std::optional<std::filesystem::path> FindSettingsPath() {
	const auto folder = FindProjectSettingsFolder();
	if (!folder) { return std::nullopt; }
	return *folder / kSettingsFileName;
}

Json ReadSettings() {
	const auto path = FindSettingsPath();
	std::error_code error;
	if (!path || !std::filesystem::exists(*path, error)) { return Json::object(); }

	auto json = JsonFile::Load(*path);
	return json && json->is_object() ? std::move(*json) : Json::object();
}

// 1 つのキーだけ書き換えて保存する（ほかのキーは残す）
void WriteSetting(std::string_view key, Json value) {
	const auto path = FindSettingsPath();
	if (!path) { return; }

	auto json = ReadSettings();
	json[std::string(key)] = std::move(value);
	JsonFile::Save(*path, json);
}

uint32_t ReadScreenSize(const Json &json, const char *key, uint32_t fallback) {
	const auto it = json.find(key);
	if (it == json.end() || !it->is_number_integer()) { return fallback; }
	return static_cast<uint32_t>(std::clamp(it->get<int64_t>(), int64_t{64}, int64_t{16384}));
}

} // namespace



void ou::PlayerSettings::SetProductName(std::string_view productName) {
	sProductName_ = productName;

	WriteSetting("productName", sProductName_); // TODO: 定数にしようね
}


const std::string &ou::PlayerSettings::GetProductName() {
	return sProductName_;
}


void ou::PlayerSettings::Load(std::string_view fallbackProductName) {
	sProductName_ = fallbackProductName.empty() ? kDefaultProductName : std::string(fallbackProductName);

	const auto json = ReadSettings();
	if (const auto it = json.find("productName"); it != json.end() && it->is_string() && !it->get<std::string>().empty()) {
		sProductName_ = it->get<std::string>();
	}

	if (const auto it = json.find("fullScreenMode"); it != json.end() && it->is_string()) {
		sFullScreenMode_ = magic_enum::enum_cast<FullScreenMode>(it->get<std::string>()).value_or(sFullScreenMode_);
	}
	sDefaultScreenWidth_ = ReadScreenSize(json, "defaultScreenWidth", sDefaultScreenWidth_);
	sDefaultScreenHeight_ = ReadScreenSize(json, "defaultScreenHeight", sDefaultScreenHeight_);
}


void ou::PlayerSettings::SetFullScreenMode(FullScreenMode mode) {
	sFullScreenMode_ = mode;
	WriteSetting("fullScreenMode", std::string(magic_enum::enum_name(mode)));
}


FullScreenMode ou::PlayerSettings::GetFullScreenMode() {
	return sFullScreenMode_;
}


void ou::PlayerSettings::SetDefaultScreenSize(uint32_t width, uint32_t height) {
	sDefaultScreenWidth_ = width;
	sDefaultScreenHeight_ = height;

	const auto path = FindSettingsPath();
	if (!path) { return; }
	auto json = ReadSettings();
	json["defaultScreenWidth"] = width;
	json["defaultScreenHeight"] = height;
	JsonFile::Save(*path, json);
}


uint32_t ou::PlayerSettings::GetDefaultScreenWidth() {
	return sDefaultScreenWidth_;
}


uint32_t ou::PlayerSettings::GetDefaultScreenHeight() {
	return sDefaultScreenHeight_;
}
