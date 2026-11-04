#include "PhysicsMaterialImporter.h"
#include "Asset/Format/AssetJson.h"
#include "Core/StringUtil.h"
#include "Diagnostics/Log.h"
#include "Physics/PhysicsMaterial.h"

#include <format>
#include <string_view>
#include <utility>

using namespace ou;
using AssetJson::Json;


namespace {

constexpr std::string_view kPhysicsMaterialKeys[] = {
	"dynamicFriction",
	"staticFriction",
	"bounciness",
	"frictionCombine",
	"bounceCombine",
};

float ToFriction(const Json &value, const char *key) {
	const auto friction = value.get<float>();
	if (friction < 0.0f) {
		throw AssetJson::FormatError(std::format("\"{}\" は 0 以上で書いてください: {}", key, friction));
	}
	return friction;
}

float ToBounciness(const Json &value) {
	const auto bounciness = value.get<float>();
	if (bounciness < 0.0f || bounciness > 1.0f) {
		throw AssetJson::FormatError(std::format("\"bounciness\" は 0~1 で書いてください: {}", bounciness));
	}
	return bounciness;
}

PhysicsMaterialCombine ToCombine(const std::string &name, const char *key) {
	if (name == "Average") { return PhysicsMaterialCombine::Average; }
	if (name == "Minimum") { return PhysicsMaterialCombine::Minimum; }
	if (name == "Multiply") { return PhysicsMaterialCombine::Multiply; }
	if (name == "Maximum") { return PhysicsMaterialCombine::Maximum; }
	throw AssetJson::FormatError(std::format("\"{}\" に \"{}\" は使えません (使えるもの: Average, Minimum, Multiply, Maximum)", key, name));
}

} // namespace


ou::PhysicsMaterialImporter::PhysicsMaterialImporter(std::string assetPath, std::filesystem::path fullPath)
	: AssetImporter(std::move(assetPath), std::move(fullPath)) {}


std::shared_ptr<Object> ou::PhysicsMaterialImporter::Import() {
	return ImportPhysicsMaterial();
}


std::shared_ptr<PhysicsMaterial> ou::PhysicsMaterialImporter::ImportPhysicsMaterial() {
	const auto &path = GetFullPath();
	const auto pathUtf8 = ToUtf8(path);

	const auto json = AssetJson::Load(path);
	if (!json) { return nullptr; }

	try {
		AssetJson::WarnUnknownKeys(*json, kPhysicsMaterialKeys, pathUtf8);

		auto material = std::make_shared<PhysicsMaterial>();
		if (const auto it = json->find("dynamicFriction"); it != json->end()) {
			material->SetDynamicFriction(ToFriction(*it, "dynamicFriction"));
		}
		if (const auto it = json->find("staticFriction"); it != json->end()) {
			material->SetStaticFriction(ToFriction(*it, "staticFriction"));
		}
		if (const auto it = json->find("bounciness"); it != json->end()) {
			material->SetBounciness(ToBounciness(*it));
		}
		if (const auto it = json->find("frictionCombine"); it != json->end()) {
			material->SetFrictionCombine(ToCombine(it->get<std::string>(), "frictionCombine"));
		}
		if (const auto it = json->find("bounceCombine"); it != json->end()) {
			material->SetBounceCombine(ToCombine(it->get<std::string>(), "bounceCombine"));
		}

		material->SetName(GetName());
		return material;

	} catch (const std::exception &e) {
		Log::Error("物理マテリアルの設定が間違っています: {}\n{}", pathUtf8, e.what());
		return nullptr;
	}
}
