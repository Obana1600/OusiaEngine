#include "MaterialImporter.h"
#include "Asset/AssetDatabase.h"
#include "Asset/AssetJson.h"
#include "Asset/Material.h"
#include "Asset/Shader.h"
#include "Asset/Texture.h"
#include "Core/Log.h"
#include "Core/Paths.h"
#include "Math/Color.h"
#include "Math/Mathf.h"
#include "Math/Vector2.h"

#include <format>
#include <string>
#include <string_view>
#include <utility>

using namespace ou;
using AssetJson::Json;


namespace {

constexpr std::string_view kMaterialKeys[] = {
	"shader",
	"renderQueue",
	"color",
	"mainTexture",
	"uvOffset",
	"uvScale",
	"uvRotation",
	"lightingMode",
	"srcBlend",
	"dstBlend",
	"blendOp",
	"zWrite",
	"cull",
};

template <typename T>
std::shared_ptr<T> LoadReferencedAsset(const Json &json, const char *key) {
	const auto assetPath = json.at(key).get<std::string>();
	auto asset = AssetDatabase::LoadAssetAtPath<T>(assetPath);
	if (!asset) {
		throw AssetJson::FormatError(std::format("\"{}\" のアセットを読めませんでした: {}", key, assetPath));
	}
	return asset;
}

// [x, y]
Vector2 ToVector2(const Json &value, const char *key) {
	if (!value.is_array() || value.size() != 2) {
		throw AssetJson::FormatError(std::format("\"{}\" は [x, y] で書いてください", key));
	}
	return {value[0].get<float>(), value[1].get<float>()};
}

// [r, g, b] or [r, g, b, a]
Color ToColor(const Json &value, const char *key) {
	if (!value.is_array() || (value.size() != 3 && value.size() != 4)) {
		throw AssetJson::FormatError(std::format("\"{}\" は [r, g, b] もしくは [r, g, b, a] と書いてください", key));
	}
	const auto a = (value.size() == 4) ? value[3].get<float>() : 1.0f;
	return {value[0].get<float>(), value[1].get<float>(), value[2].get<float>(), a};
}

Material::LightingMode ToLightingMode(const std::string &name) {
	if (name == "None") { return Material::LightingMode::None; }
	if (name == "Lambert") { return Material::LightingMode::Lambert; }
	if (name == "HalfLambert") { return Material::LightingMode::HalfLambert; }
	throw AssetJson::FormatError(std::format("\"lightingMode\" に \"{}\" は使えません (使えるもの: None, Lambert, HalfLambert)", name));
}

Shader::RenderStateOverride ParseRenderStateOverride(const Json &json) {
	Shader::RenderStateOverride override{};
	if (const auto it = json.find("srcBlend"); it != json.end()) {
		override.srcBlend = AssetJson::ToBlend(it->get<std::string>(), "srcBlend");
	}
	if (const auto it = json.find("dstBlend"); it != json.end()) {
		override.dstBlend = AssetJson::ToBlend(it->get<std::string>(), "dstBlend");
	}
	if (const auto it = json.find("blendOp"); it != json.end()) {
		override.blendOp = AssetJson::ToBlendOp(it->get<std::string>(), "blendOp");
	}
	if (const auto it = json.find("zWrite"); it != json.end()) {
		override.zWrite = it->get<bool>();
	}
	if (const auto it = json.find("cull"); it != json.end()) {
		override.cull = AssetJson::ToCullMode(it->get<std::string>(), "cull");
	}
	return override;
}

} // namespace



ou::MaterialImporter::MaterialImporter(std::string assetPath, std::filesystem::path fullPass)
	: AssetImporter(std::move(assetPath), std::move(fullPass)) {}


std::shared_ptr<Object> ou::MaterialImporter::Import() {
	return ImportMaterial();
}


std::shared_ptr<Material> ou::MaterialImporter::ImportMaterial() {
	const auto &path = GetAssetPath();
	const auto pathUtf8 = ToUtf8(path);

	const auto json = AssetJson::Load(path);
	if (!json) { return nullptr; }

	try {
		AssetJson::WarnUnknownKeys(*json, kMaterialKeys, pathUtf8);

		// シェーダ
		std::shared_ptr<Shader> shader;
		if (json->contains("shader")) {
			shader = LoadReferencedAsset<Shader>(*json, "shader");
		}
		auto material = Material::Create(std::move(shader));
		material->SetName(GetName());

		// プロパティ
		if (const auto it = json->find("renderQueue"); it != json->end()) {
			material->SetRenderQueue(AssetJson::ToRenderQueue(*it, "renderQueue"));
		}
		if (const auto it = json->find("color"); it != json->end()) {
			material->SetColor(ToColor(*it, "color"));
		}
		if (json->contains("mainTexture")) {
			material->SetMainTexture(LoadReferencedAsset<Texture>(*json, "mainTexture"));
		}
		if (const auto it = json->find("uvOffset"); it != json->end()) {
			material->SetUvOffset(ToVector2(*it, "uvOffset"));
		}
		if (const auto it = json->find("uvScale"); it != json->end()) {
			material->SetUvScale(ToVector2(*it, "uvScale"));
		}
		if (const auto it = json->find("uvRotation"); it != json->end()) {
			material->SetUvRotation(it->get<float>() * Mathf::Deg2Rad);
		}
		if (const auto it = json->find("lightingMode"); it != json->end()) {
			material->SetLightingMode(ToLightingMode(it->get<std::string>()));
		}

		material->SetRenderStateOverride(ParseRenderStateOverride(*json));
		return material;

	} catch (const std::exception &e) {
		Log::Error("マテリアルの設定が間違っています: {}\n{}", pathUtf8, e.what());
		return nullptr;
	}
}
