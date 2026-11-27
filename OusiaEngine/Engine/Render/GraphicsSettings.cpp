#include "GraphicsSettings.h"
#include "Core/JsonFile.h"
#include "Core/StringUtil.h"
#include "Diagnostics/Log.h"
#include "Platform/Paths.h"
#include "PostProcess/VolumeProfile.h"
#include "Serialize/JsonReader.h"
#include "Serialize/JsonWriter.h"

#include <cstdint>
#include <filesystem>
#include <optional>
#include <string_view>
#include <system_error>
#include <unordered_map>
#include <unordered_set>

using namespace ou;


namespace {

constexpr const wchar_t *kFileName = L"GraphicsSettings.json";

constexpr std::string_view kKeys[] = {
	"defaultVolumeProfile",
	"rendererFeatures",
	"opaqueDownsampling",
	"mainLightShadowmapResolution",
	"shadowDistance",
};

std::optional<std::filesystem::path> FindPath() {
	const auto folder = FindProjectSettingsFolder();
	if (!folder) { return std::nullopt; }
	return *folder / kFileName;
}

} // namespace



void ou::GraphicsSettings::Serialize(Serializer &serializer) {
	serializer.Asset("defaultVolumeProfile", defaultVolumeProfile_);
	serializer.Array("rendererFeatures", rendererFeatures_, [&](FullScreenPassRendererFeature &feature) {
		serializer.Struct("", [&] { feature.Serialize(serializer); });
	});

	serializer.Enum("opaqueDownsampling", opaqueDownsampling_);
	serializer.Enum("mainLightShadowmapResolution", mainLightShadowmapResolution_);
	serializer.Field("shadowDistance", shadowDistance_);
}


GraphicsSettings &ou::GraphicsSettings::GetInstance() {
	static GraphicsSettings instance;
	return instance;
}


void ou::GraphicsSettings::Load() {
	const auto path = FindPath();
	std::error_code error;
	if (!path || !std::filesystem::exists(*path, error)) { return; }

	const auto json = JsonFile::Load(*path);
	if (!json) { return; }

	const auto pathUtf8 = ToUtf8(*path);
	if (!json->is_object()) {
		Log::Warning("GraphicsSettings は {{ }} で書いてください: {}", pathUtf8);
		return;
	}
	JsonFile::WarnUnknownKeys(*json, kKeys, pathUtf8);

	const std::unordered_map<uint64_t, Object *> noSceneObjects;
	JsonReader reader(*json, noSceneObjects);
	GetInstance().Serialize(reader);
}


void ou::GraphicsSettings::Save() {
	const auto path = FindPath();
	if (!path) { return; }

	auto json = Json::object();
	const std::unordered_set<const Object *> noSceneObjects;
	JsonWriter writer(json, noSceneObjects);
	GetInstance().Serialize(writer);
	JsonFile::Save(*path, json);
}


void ou::GraphicsSettings::Unload() {
	GetInstance().defaultVolumeProfile_.reset();
	GetInstance().rendererFeatures_.clear();
	GetInstance().opaqueDownsampling_ = Downsampling::_2xBilinear;
}
