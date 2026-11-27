#include "ShaderImporter.h"
#include "Asset/AssetDatabase.h"
#include "Asset/Format/AssetJson.h"
#include "Core/JsonFile.h"
#include "Core/StringUtil.h"
#include "Diagnostics/Log.h"
#include "Render/Asset/Shader.h"
#include "Render/Asset/ShaderModule.h"

#include <format>
#include <string>
#include <string_view>
#include <utility>

using namespace ou;


namespace {

constexpr std::string_view kShaderKeys[] = {
	"queue",
	"passes",
	"properties",
};

constexpr std::string_view kPassKeys[] = {
	"name",
	"tags",
	"vertex",
	"pixel",
	"hull",
	"domain",
	"geometry",
	"srcBlend",
	"dstBlend",
	"blendOp",
	"zWrite",
	"zTest",
	"cull",
};

constexpr std::string_view kPassTagKeys[] = {
	"LightMode",
};

constexpr std::string_view kPropertyKeys[] = {
	"name",
	"type",
	"default",
};

Graphics::Shader ImportStage(
	const AssetImporter &importer,
	const Json &pass,
	const char *key,
	Graphics::ShaderStage stage,
	bool required) {
	// キーが存在するか調べる
	const auto it = pass.find(key);
	if (it == pass.end()) {
		if (required) {
			throw JsonFile::FormatError(std::format("Pass に \"{}\" がありません", key));
		}
		return {};
	}

	// シェーダーアセットを作る
	const auto fileName = it->get<std::string>();
	const auto isAssetPath = fileName.starts_with("EngineAssets/") || fileName.starts_with("Assets/");
	const auto module = AssetDatabase::LoadAssetAtPath<ShaderModule>(isAssetPath ? fileName : importer.ResolveAssetPath(fileName));
	if (!module) {
		throw JsonFile::FormatError(std::format("\"{}\" の HLSL のコンパイルに失敗しました: {}", key, fileName));
	}

	// キーと同じステージか調べる
	if (module->GetStage() != stage) {
		throw JsonFile::FormatError(std::format("\"{}\" に書けるのは、このステージの HLSL だけです: {}", key, fileName));
	}

	return module->GetGraphicsShader();
}

Shader::Pass ParsePass(const AssetImporter &importer, const Json &json) {
	Shader::Pass pass{};
	pass.name = json.value("name", std::string());

	// タグ
	if (const auto it = json.find("tags"); it != json.end()) {
		if (!it->is_object()) {
			throw JsonFile::FormatError("\"tags\" は {\"LightMode\": \"...\"} のように書いてください");
		}
		if (const auto lightMode = it->find("LightMode"); lightMode != it->end()) {
			if (!lightMode->is_string()) {
				throw JsonFile::FormatError("\"LightMode\" は文字列で書いてください");
			}
			pass.lightMode = lightMode->get<std::string>();
		}
	}

	// シェーダー
	pass.vertexShader = ImportStage(importer, json, "vertex", Graphics::ShaderStage::VS, true);
	pass.pixelShader = ImportStage(importer, json, "pixel", Graphics::ShaderStage::PS, false);
	pass.hullShader = ImportStage(importer, json, "hull", Graphics::ShaderStage::HS, false);
	pass.domainShader = ImportStage(importer, json, "domain", Graphics::ShaderStage::DS, false);
	pass.geometryShader = ImportStage(importer, json, "geometry", Graphics::ShaderStage::GS, false);

	// ブレンド
	auto &target = pass.blend.renderTarget[0];
	target.srcBlend = AssetJson::ToBlend(json.value("srcBlend", std::string("One")), "srcBlend");
	target.destBlend = AssetJson::ToBlend(json.value("dstBlend", std::string("Zero")), "dstBlend");
	target.blendOp = AssetJson::ToBlendOp(json.value("blendOp", std::string("Add")), "blendOp");
	target.blendEnable = !(target.srcBlend == Graphics::Blend::ONE &&
						   target.destBlend == Graphics::Blend::ZERO &&
						   target.blendOp == Graphics::BlendOp::ADD);

	// 深度ステンシル
	pass.depthStencil.depthEnable = true;
	pass.depthStencil.depthWriteMask = json.value("zWrite", true) ? Graphics::DepthWriteMask::ALL
																  : Graphics::DepthWriteMask::ZERO;
	pass.depthStencil.comparisonFunc = AssetJson::ToComparisonFunc(json.value("zTest", std::string("LEqual")), "zTest");

	// ラスタライザ
	pass.rasterizer.cullMode = AssetJson::ToCullMode(json.value("cull", std::string("Back")), "cull");

	return pass;
}

Shader::PropertyType ToPropertyType(const std::string &name) {
	if (name == "Color") { return Shader::PropertyType::Color; }
	if (name == "Vector") { return Shader::PropertyType::Vector; }
	if (name == "Float") { return Shader::PropertyType::Float; }
	if (name == "Texture") { return Shader::PropertyType::Texture; }

	throw JsonFile::FormatError(std::format("\"type\" に \"{}\" は使えません (使えるもの: Color, Vector, Float, Texture)", name));
}

Vector4 ToDefaultValue(const Json &value, Shader::PropertyType type) {
	if (type == Shader::PropertyType::Texture) {
		throw JsonFile::FormatError("\"Texture\" には \"default\" を書けません");
	}

	if (type == Shader::PropertyType::Float) {
		if (!value.is_number()) {
			throw JsonFile::FormatError("\"Float\" の \"default\" は数で書いてください");
		}
		return {value.get<float>(), 0.0f, 0.0f, 0.0f};
	}

	if (!value.is_array() || value.size() != 4) {
		throw JsonFile::FormatError("\"Color\" と \"Vector\" の \"default\" は [x, y, z, w] と書いてください");
	}

	return {value[0].get<float>(), value[1].get<float>(), value[2].get<float>(), value[3].get<float>()};
}

Shader::Property ParseProperty(const Json &json) {
	Shader::Property property{};
	property.name = json.at("name").get<std::string>();
	property.type = ToPropertyType(json.at("type").get<std::string>());
	if (const auto it = json.find("default"); it != json.end()) {
		property.defaultValue = ToDefaultValue(*it, property.type);
	}
	return property;
}

} // namespace



ou::ShaderImporter::ShaderImporter(std::string assetPath, std::filesystem::path fullPath)
	: AssetImporter(std::move(assetPath), std::move(fullPath)) {}


std::shared_ptr<Object> ou::ShaderImporter::Import() {
	return ImportShader();
}


std::shared_ptr<Shader> ou::ShaderImporter::ImportShader() {
	const auto &path = GetFullPath();
	const auto pathUtf8 = ToUtf8(path);

	const auto json = JsonFile::Load(path);
	if (!json) { return nullptr; }

	try {
		JsonFile::WarnUnknownKeys(*json, kShaderKeys, pathUtf8);

		// Render Queue
		auto renderQueue = Shader::RenderQueue::Geometry;
		if (const auto it = json->find("queue"); it != json->end()) {
			renderQueue = AssetJson::ToRenderQueue(*it, "queue");
		}

		// Pass
		const auto &passesJson = json->at("passes");
		if (!passesJson.is_array() || passesJson.empty()) {
			throw JsonFile::FormatError("\"passes\" には Pass を 1 つ以上、配列で書いてください");
		}

		std::vector<Shader::Pass> passes;
		for (const auto &passJson : passesJson) {
			if (!passJson.is_object()) {
				throw JsonFile::FormatError("\"passes\" の中身は {...} と書いてください");
			}
			JsonFile::WarnUnknownKeys(passJson, kPassKeys, pathUtf8);
			if (const auto it = passJson.find("tags"); it != passJson.end() && it->is_object()) {
				JsonFile::WarnUnknownKeys(*it, kPassTagKeys, pathUtf8);
			}
			passes.push_back(ParsePass(*this, passJson));
		}

		// Property
		std::vector<Shader::Property> properties;
		if (const auto it = json->find("properties"); it != json->end()) {
			if (!it->is_array()) {
				throw JsonFile::FormatError("\"properties\" は配列で書いてください");
			}

			for (const auto &propertyJson : *it) {
				if (!propertyJson.is_object()) {
					throw JsonFile::FormatError("\"properties\" の中身は {...} と書いてください");
				}
				JsonFile::WarnUnknownKeys(propertyJson, kPropertyKeys, pathUtf8);
				properties.push_back(ParseProperty(propertyJson));
			}
		}

		auto shader = std::make_shared<Shader>(std::move(passes), renderQueue, std::move(properties));
		shader->SetName(GetName());
		return shader;

	} catch (const std::exception &e) {
		Log::Error("シェーダの設定が間違っています: {}\n{}", pathUtf8, e.what());
		return nullptr;
	}
}
