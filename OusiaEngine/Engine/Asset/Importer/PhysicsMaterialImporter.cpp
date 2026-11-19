#include "PhysicsMaterialImporter.h"
#include "Core/JsonFile.h"
#include "Core/StringUtil.h"
#include "Diagnostics/Log.h"
#include "Physics/PhysicsMaterial.h"
#include "Serialize/JsonReader.h"

#include <cstdint>
#include <unordered_map>
#include <utility>

using namespace ou;



ou::PhysicsMaterialImporter::PhysicsMaterialImporter(std::string assetPath, std::filesystem::path fullPath)
	: AssetImporter(std::move(assetPath), std::move(fullPath)) {}


std::shared_ptr<Object> ou::PhysicsMaterialImporter::Import() {
	return ImportPhysicsMaterial();
}


std::shared_ptr<PhysicsMaterial> ou::PhysicsMaterialImporter::ImportPhysicsMaterial() {
	const auto json = JsonFile::Load(GetFullPath());
	if (!json) { return nullptr; }
	if (!json->is_object()) {
		Log::Error("物理マテリアルは {{...}} で書いてください: {}", ToUtf8(GetFullPath()));
		return nullptr;
	}

	// 中身は PhysicsMaterial::Serialize と同じ形（エディタの保存も同じ形で書く）
	auto material = std::make_shared<PhysicsMaterial>();
	const std::unordered_map<uint64_t, Object *> noSceneObjects;
	JsonReader reader(*json, noSceneObjects);
	material->Serialize(reader);

	material->SetName(GetName());
	return material;
}
