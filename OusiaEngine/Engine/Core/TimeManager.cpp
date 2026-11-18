#include "TimeManager.h"
#include "Core/JsonFile.h"
#include "Core/StringUtil.h"
#include "Core/Time.h"
#include "Diagnostics/Log.h"
#include "Platform/Paths.h"

#include <algorithm>
#include <filesystem>
#include <optional>
#include <string>
#include <string_view>
#include <system_error>

using namespace ou;


namespace {

constexpr const wchar_t *kFileName = L"TimeManager.json";

constexpr std::string_view kKeys[] = {
	"fixedTimestep",
	"maximumAllowedTimestep",
	"timeScale",
};

// Time と同じ下限（0 にすると FixedUpdate が終わらない）
constexpr float kMinTimestep = 0.0001f;
// Unity の Time Scale の上限
constexpr float kMaxTimeScale = 100.0f;

std::optional<std::filesystem::path> FindPath() {
	const auto folder = FindProjectSettingsFolder();
	if (!folder) { return std::nullopt; }
	return *folder / kFileName;
}

void ReadFloat(const Json &json, const char *key, float &value, float min, float max, const std::string &pathUtf8) {
	const auto it = json.find(key);
	if (it == json.end()) { return; }

	if (!it->is_number()) {
		Log::Warning("\"{}\" は数で書いてください: {}", key, pathUtf8);
		return;
	}
	value = std::clamp(it->get<float>(), min, max);
}

} // namespace



void ou::TimeManager::SetFixedTimestep(float fixedTimestep) {
	sFixedTimestep_ = std::max(fixedTimestep, kMinTimestep);
	Time::SetFixedDeltaTime(sFixedTimestep_);
}


float ou::TimeManager::GetFixedTimestep() {
	return sFixedTimestep_;
}


void ou::TimeManager::SetMaximumAllowedTimestep(float maximumAllowedTimestep) {
	sMaximumAllowedTimestep_ = std::max(maximumAllowedTimestep, kMinTimestep);
	Time::SetMaximumDeltaTime(sMaximumAllowedTimestep_);
}


float ou::TimeManager::GetMaximumAllowedTimestep() {
	return sMaximumAllowedTimestep_;
}


void ou::TimeManager::SetTimeScale(float timeScale) {
	sTimeScale_ = std::clamp(timeScale, 0.0f, kMaxTimeScale);
	Time::SetTimeScale(sTimeScale_);
}


float ou::TimeManager::GetTimeScale() {
	return sTimeScale_;
}


void ou::TimeManager::Save() {
	const auto path = FindPath();
	if (!path) { return; }

	Json json = Json::object();
	json["fixedTimestep"] = sFixedTimestep_;
	json["maximumAllowedTimestep"] = sMaximumAllowedTimestep_;
	json["timeScale"] = sTimeScale_;
	JsonFile::Save(*path, json);
}


void ou::TimeManager::Apply() {
	Time::SetFixedDeltaTime(sFixedTimestep_);
	Time::SetMaximumDeltaTime(sMaximumAllowedTimestep_);
	Time::SetTimeScale(sTimeScale_);
}


void ou::TimeManager::Load() {
	const auto path = FindPath();
	std::error_code error;
	if (path && std::filesystem::exists(*path, error)) {
		if (const auto json = JsonFile::Load(*path)) {
			const auto pathUtf8 = ToUtf8(*path);
			if (json->is_object()) {
				JsonFile::WarnUnknownKeys(*json, kKeys, pathUtf8);
				ReadFloat(*json, "fixedTimestep", sFixedTimestep_, kMinTimestep, 1.0f, pathUtf8);
				ReadFloat(*json, "maximumAllowedTimestep", sMaximumAllowedTimestep_, kMinTimestep, 1.0f, pathUtf8);
				ReadFloat(*json, "timeScale", sTimeScale_, 0.0f, kMaxTimeScale, pathUtf8);
			} else {
				Log::Warning("TimeManager は {{ }} で書いてください: {}", pathUtf8);
			}
		}
	}

	Apply();
}
