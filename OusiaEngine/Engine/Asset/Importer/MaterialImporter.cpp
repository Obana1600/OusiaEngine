#include "MaterialImporter.h"
#include "Core/JsonFile.h"
#include "Core/StringUtil.h"
#include "Diagnostics/Log.h"
#include "Render/Asset/Material.h"
#include "Serialize/JsonReader.h"

#include <cstdint>
#include <unordered_map>
#include <utility>

using namespace ou;



ou::MaterialImporter::MaterialImporter(std::string assetPath, std::filesystem::path fullPath)
	: AssetImporter(std::move(assetPath), std::move(fullPath)) {}


std::shared_ptr<Object> ou::MaterialImporter::Import() {
	return ImportMaterial();
}


std::shared_ptr<Material> ou::MaterialImporter::ImportMaterial() {
	const auto json = JsonFile::Load(GetFullPath());
	if (!json) { return nullptr; }
	if (!json->is_object()) {
		Log::Error("マテリアルは {{...}} で書いてください: {}", ToUtf8(GetFullPath()));
		return nullptr;
	}

	// 中身は Material::Serialize と同じ形（エディタの保存も同じ形で書く）。書いていないものは Material::Create の値のまま
	auto material = Material::Create();
	const std::unordered_map<uint64_t, Object *> noSceneObjects;
	JsonReader reader(*json, noSceneObjects);
	material->Serialize(reader);

	material->SetName(GetName());
	return material;
}
