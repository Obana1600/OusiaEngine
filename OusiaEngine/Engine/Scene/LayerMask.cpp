#include "LayerMask.h"
#include "Asset/Format/AssetJson.h"
#include "Core/StringUtil.h"
#include "Diagnostics/Log.h"
#include "Platform/Paths.h"

#include <algorithm>
#include <array>
#include <charconv>
#include <filesystem>
#include <utility>

using namespace ou;
using AssetJson::Json;


namespace {

constexpr std::string_view kTagManagerKeys[] = {
	"layers",
};

constexpr std::pair<int, std::string_view> kBuiltinLayers[] = {
	{0, "Default"},
	{1, "Ignore Raycast"},
	{2, "UI"},
};

std::array<std::string, LayerMask::kLayerCount> &LayerNames() {
	static std::array<std::string, LayerMask::kLayerCount> names;
	return names;
}

bool IsBuiltinLayer(int layer) {
	for (const auto &[builtin, name] : kBuiltinLayers) {
		if (builtin == layer) { return true; }
	}
	return false;
}

int ParseLayerIndex(std::string_view text) {
	int layer = -1;
	const auto [end, error] = std::from_chars(text.data(), text.data() + text.size(), layer);
	if (error != std::errc() || end != text.data() + text.size()) { return -1; }
	return layer;
}

} // namespace



int ou::LayerMask::NameToLayer(std::string_view layerName) {
	const auto &names = LayerNames();
	for (int i = 0; i < kLayerCount; i++) {
		if (!names[i].empty() && names[i] == layerName) { return i; }
	}
	return -1;
}


std::string ou::LayerMask::LayerToName(int layer) {
	if (layer < 0 || layer >= kLayerCount) { return {}; }
	return LayerNames()[layer];
}


uint32_t ou::LayerMask::GetMask(std::initializer_list<std::string_view> layerNames) {
	uint32_t mask = 0;
	for (const auto name : layerNames) {
		const auto layer = NameToLayer(name);
		if (layer >= 0) { mask |= 1u << layer; }
	}
	return mask;
}


void ou::LayerMask::LoadLayerNames() {
	auto &names = LayerNames();
	names = {};
	for (const auto &[layer, name] : kBuiltinLayers) {
		names[layer] = name;
	}

	const auto folder = FindProjectSettingsFolder();
	if (!folder) { return; }
	const auto path = *folder / L"TagManager.json";
	if (!std::filesystem::exists(path)) { return; }

	const auto json = AssetJson::Load(path);
	if (!json) { return; }

	const auto pathUtf8 = ToUtf8(path);
	try {
		AssetJson::WarnUnknownKeys(*json, kTagManagerKeys, pathUtf8);

		const auto it = json->find("layers");
		if (it == json->end()) { return; }

		for (const auto &[key, value] : it->items()) {
			const auto layer = ParseLayerIndex(key);
			if (layer < 0 || layer >= kLayerCount) {
				Log::Warning("レイヤーの番号は 0~31 で書いてください: {}: \"{}\"", pathUtf8, key);
				continue;
			}
			if (IsBuiltinLayer(layer)) {
				Log::Warning("組み込みのレイヤー {} ({}) は変えられません: {}", layer, names[layer], pathUtf8);
				continue;
			}

			const auto name = value.get<std::string>();
			if (name.empty()) { continue; }
			if (const auto other = NameToLayer(name); other >= 0) {
				Log::Warning("レイヤーの名前 \"{}\" が {} と {} で重複しています: {}", name, other, layer, pathUtf8);
			}
			names[layer] = name;
		}

	} catch (const Json::exception &e) {
		Log::Error("TagManager.json の読み込みに失敗しました: {}\n{}", pathUtf8, e.what());
	}
}


ou::LayerMask::operator uint32_t() const {
	return value;
}
