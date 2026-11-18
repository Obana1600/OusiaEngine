#include "TagManager.h"
#include "Core/JsonFile.h"
#include "Core/StringUtil.h"
#include "Diagnostics/Log.h"
#include "Platform/Paths.h"

#include <algorithm>
#include <filesystem>
#include <system_error>

using namespace ou;


namespace {

constexpr std::string_view kBuiltinTags[] = {
	TagManager::kUntagged,
	TagManager::kEditorOnly,
	TagManager::kMainCamera,
	TagManager::kPlayer,
};

std::vector<std::string> &Tags() {
	static std::vector<std::string> tags(std::begin(kBuiltinTags), std::end(kBuiltinTags));
	return tags;
}

std::string Trim(std::string_view text) {
	const auto first = text.find_first_not_of(" \t");
	if (first == std::string_view::npos) { return {}; }
	const auto last = text.find_last_not_of(" \t");
	return std::string(text.substr(first, last - first + 1));
}

} // namespace



const std::vector<std::string> &ou::TagManager::GetTags() {
	return Tags();
}


bool ou::TagManager::IsBuiltinTag(std::string_view tag) {
	return std::find(std::begin(kBuiltinTags), std::end(kBuiltinTags), tag) != std::end(kBuiltinTags);
}


bool ou::TagManager::IsDefined(std::string_view tag) {
	const auto &tags = Tags();
	return std::find(tags.begin(), tags.end(), tag) != tags.end();
}


std::string ou::TagManager::AddTag(std::string_view tag) {
	const auto name = Trim(tag);
	if (name.empty()) { return "タグの名前を入れてください"; }
	if (IsDefined(name)) { return "\"" + name + "\" はもうあります"; }

	Tags().push_back(name);
	Save();
	return {};
}


void ou::TagManager::RemoveTag(std::string_view tag) {
	if (IsBuiltinTag(tag)) { return; }

	auto &tags = Tags();
	const auto it = std::find(tags.begin(), tags.end(), tag);
	if (it == tags.end()) { return; }

	tags.erase(it);
	Save();
}


void ou::TagManager::Load() {
	auto &tags = Tags();
	tags.assign(std::begin(kBuiltinTags), std::end(kBuiltinTags));

	const auto folder = FindProjectSettingsFolder();
	if (!folder) { return; }
	const auto path = *folder / L"TagManager.json";
	std::error_code error;
	if (!std::filesystem::exists(path, error)) { return; }

	const auto json = JsonFile::Load(path);
	if (!json || !json->is_object()) { return; }

	const auto it = json->find("tags");
	if (it == json->end()) { return; }

	const auto pathUtf8 = ToUtf8(path);
	if (!it->is_array()) {
		Log::Warning("\"tags\" は [\"名前\", ...] で書いてください: {}", pathUtf8);
		return;
	}

	for (const auto &value : *it) {
		if (!value.is_string()) {
			Log::Warning("\"tags\" は [\"名前\", ...] で書いてください: {}", pathUtf8);
			continue;
		}

		const auto name = Trim(value.get<std::string>());
		if (name.empty()) { continue; }
		if (IsDefined(name)) {
			// 組み込みのタグを書いていても、重ねて足さない
			if (!IsBuiltinTag(name)) { Log::Warning("タグ \"{}\" が重複しています: {}", name, pathUtf8); }
			continue;
		}
		tags.push_back(name);
	}
}


void ou::TagManager::Save() {
	const auto folder = FindProjectSettingsFolder();
	if (!folder) { return; }
	const auto path = *folder / L"TagManager.json";

	// レイヤーの名前も同じファイルにあるので、"tags" だけを書き換える
	Json json = Json::object();
	std::error_code error;
	if (std::filesystem::exists(path, error)) {
		if (auto loaded = JsonFile::Load(path); loaded && loaded->is_object()) { json = std::move(*loaded); }
	}

	Json tags = Json::array();
	for (const auto &tag : Tags()) {
		if (!IsBuiltinTag(tag)) { tags.push_back(tag); }
	}
	json["tags"] = std::move(tags);
	JsonFile::Save(path, json);
}
