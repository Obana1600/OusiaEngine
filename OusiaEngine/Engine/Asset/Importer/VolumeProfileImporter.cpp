#include "VolumeProfileImporter.h"
#include "Core/JsonFile.h"
#include "Core/StringUtil.h"
#include "Diagnostics/Log.h"
#include "PostProcess/VolumeProfile.h"
#include "Serialize/JsonReader.h"

#include <cstdint>
#include <unordered_map>
#include <utility>

using namespace ou;


ou::VolumeProfileImporter::VolumeProfileImporter(std::string assetPath, std::filesystem::path fullPath)
	: AssetImporter(std::move(assetPath), std::move(fullPath)) {}


std::shared_ptr<Object> ou::VolumeProfileImporter::Import() {
	return ImportVolumeProfile();
}


std::shared_ptr<VolumeProfile> ou::VolumeProfileImporter::ImportVolumeProfile() {
	const auto json = JsonFile::Load(GetFullPath());
	if (!json) { return nullptr; }
	if (!json->is_object()) {
		Log::Error("Volume Profile は {{...}} で書いてください: {}", ToUtf8(GetFullPath()));
		return nullptr;
	}

	auto profile = std::make_shared<VolumeProfile>();
	const std::unordered_map<uint64_t, Object *> noSceneObjects;
	JsonReader reader(*json, noSceneObjects);
	profile->Serialize(reader);

	profile->SetName(GetName());
	return profile;
}
