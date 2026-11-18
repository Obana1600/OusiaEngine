#include "EditorPrefs.h"
#include "Core/JsonFile.h"
#include "Core/StringUtil.h"
#include "Diagnostics/Log.h"
#include "Platform/Paths.h"

#include <filesystem>
#include <string>
#include <system_error>

using namespace ou;


namespace {

std::filesystem::path PrefsPath() {
	return LocalAppDataDirectory() / L"OusiaEngine" / L"EditorPrefs.json";
}

// 初めて使うときに読み、変えるたびに書く（数が少ないので丸ごと書いてよい）
Json &Prefs() {
	static Json prefs = [] {
		std::error_code error;
		if (!std::filesystem::exists(PrefsPath(), error)) { return Json::object(); }

		auto json = JsonFile::Load(PrefsPath());
		if (!json || !json->is_object()) {
			Log::Warning("EditorPrefs.json を読めなかったので、空にします: {}", ToUtf8(PrefsPath()));
			return Json::object();
		}
		return std::move(*json);
	}();
	return prefs;
}

void Save() {
	JsonFile::Save(PrefsPath(), Prefs());
}

template <typename T>
T GetValue(std::string_view key, T defaultValue) {
	const auto &prefs = Prefs();
	const auto it = prefs.find(std::string(key));
	if (it == prefs.end()) { return defaultValue; }

	try {
		return it->get<T>();
	} catch (const Json::exception &) {
		return defaultValue;
	}
}

template <typename T>
void SetValue(std::string_view key, const T &value) {
	Prefs()[std::string(key)] = value;
	Save();
}

} // namespace



std::string ou::EditorPrefs::GetString(std::string_view key, std::string_view defaultValue) {
	return GetValue<std::string>(key, std::string(defaultValue));
}


void ou::EditorPrefs::SetString(std::string_view key, std::string_view value) {
	SetValue(key, std::string(value));
}


int ou::EditorPrefs::GetInt(std::string_view key, int defaultValue) {
	return GetValue<int>(key, defaultValue);
}


void ou::EditorPrefs::SetInt(std::string_view key, int value) {
	SetValue(key, value);
}


bool ou::EditorPrefs::GetBool(std::string_view key, bool defaultValue) {
	return GetValue<bool>(key, defaultValue);
}


void ou::EditorPrefs::SetBool(std::string_view key, bool value) {
	SetValue(key, value);
}


bool ou::EditorPrefs::HasKey(std::string_view key) {
	return Prefs().contains(std::string(key));
}


void ou::EditorPrefs::DeleteKey(std::string_view key) {
	Prefs().erase(std::string(key));
	Save();
}
