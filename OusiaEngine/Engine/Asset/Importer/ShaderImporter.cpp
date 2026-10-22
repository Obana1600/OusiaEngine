#include "ShaderImporter.h"
#include "Asset/AssetDatabase.h"
#include "Asset/Format/AssetJson.h"
#include "Core/StringUtil.h"
#include "Diagnostics/Log.h"
#include "Render/Asset/Shader.h"
#include "Render/Asset/ShaderModule.h"

#include <format>
#include <string>
#include <string_view>
#include <utility>

#include <nlohmann/json.hpp>

using namespace ou;
using ou::AssetJson::Json;


namespace {

constexpr std::string_view kShaderKeys[] = {
	"queue",
	"passes",
};

constexpr std::string_view kPassKeys[] = {
	"name",
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
			throw AssetJson::FormatError(std::format("Pass に \"{}\" がありません", key));
		}
		return {};
	}

	// シェーダーアセットを作る
	const auto fileName = it->get<std::string>();
	const auto module = AssetDatabase::LoadAssetAtPath<ShaderModule>(importer.ResolveAssetPath(fileName));
	if (!module) {
		throw AssetJson::FormatError(std::format("\"{}\" の HLSL をコンパイルできませんでした: {}", key, fileName));
	}

	// キーと同じステージか調べる
	if (module->GetStage() != stage) {
		throw AssetJson::FormatError(std::format("\"{}\" に書けるのは、このステージの HLSL だけです: {}", key, fileName));
	}

	return module->GetGraphicsShader();
}

Shader::Pass ParsePass(const AssetImporter &importer, const Json &json) {
	Shader::Pass pass{};
	pass.name = json.value("name", std::string());

	// シェーダー
	pass.vertexShader = ImportStage(importer, json, "vertex", Graphics::ShaderStage::VS, true);
	pass.pixelShader = ImportStage(importer, json, "pixel", Graphics::ShaderStage::PS, true);
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

} // namespace



ou::ShaderImporter::ShaderImporter(std::string assetPath, std::filesystem::path fullPath)
	: AssetImporter(std::move(assetPath), std::move(fullPath)) {}


std::shared_ptr<Object> ou::ShaderImporter::Import() {
	return ImportShader();
}


std::shared_ptr<Shader> ou::ShaderImporter::ImportShader() {
	const auto &path = GetFullPath();
	const auto pathUtf8 = ToUtf8(path);

	const auto json = AssetJson::Load(path);
	if (!json) { return nullptr; }

	try {
		AssetJson::WarnUnknownKeys(*json, kShaderKeys, pathUtf8);

		// Render Queue
		auto renderQueue = Shader::RenderQueue::Geometry;
		if (const auto it = json->find("queue"); it != json->end()) {
			renderQueue = AssetJson::ToRenderQueue(*it, "queue");
		}

		// Pass
		const auto &passesJson = json->at("passes");
		if (!passesJson.is_array() || passesJson.empty()) {
			throw AssetJson::FormatError("\"passes\" には Pass を 1 つ以上、配列で書いてください");
		}

		std::vector<Shader::Pass> passes;
		for (const auto &passJson : passesJson) {
			if (!passJson.is_object()) {
				throw AssetJson::FormatError("\"passes\" の中身は {...} と書いてください");
			}
			AssetJson::WarnUnknownKeys(passJson, kPassKeys, pathUtf8);
			passes.push_back(ParsePass(*this, passJson));
		}

		auto shader = std::make_shared<Shader>(std::move(passes), renderQueue);
		shader->SetName(GetName());
		return shader;

	} catch (const std::exception &e) {
		Log::Error("シェーダの設定が間違っています: {}\n{}", pathUtf8, e.what());
		return nullptr;
	}
}
