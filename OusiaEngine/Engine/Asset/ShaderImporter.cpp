#include "ShaderImporter.h"
#include "Asset/Shader.h"
#include "Asset/ShaderModule.h"
#include "Asset/ShaderModuleImporter.h"
#include "Core/Log.h"
#include "Core/Paths.h"

#include <nlohmann/json.hpp>

#include <algorithm>
#include <format>
#include <fstream>
#include <optional>
#include <stdexcept>
#include <string>
#include <string_view>
#include <utility>

using namespace ou;
using Json = nlohmann::json;


namespace {

template <typename T>
struct NamedValue {
	std::string_view name;
	T value;
};

constexpr NamedValue<Graphics::Blend> kBlendFactors[] = {
	{"Zero", Graphics::Blend::ZERO},
	{"One", Graphics::Blend::ONE},
	{"SrcColor", Graphics::Blend::SRC_COLOR},
	{"OneMinusSrcColor", Graphics::Blend::INV_SRC_COLOR},
	{"SrcAlpha", Graphics::Blend::SRC_ALPHA},
	{"OneMinusSrcAlpha", Graphics::Blend::INV_SRC_ALPHA},
	{"DstColor", Graphics::Blend::DEST_COLOR},
	{"OneMinusDstColor", Graphics::Blend::INV_DEST_COLOR},
	{"DstAlpha", Graphics::Blend::DEST_ALPHA},
	{"OneMinusDstAlpha", Graphics::Blend::INV_DEST_ALPHA},
};

constexpr NamedValue<Graphics::BlendOp> kBlendOps[] = {
	{"Add", Graphics::BlendOp::ADD},
	{"Sub", Graphics::BlendOp::SUBTRACT},
	{"RevSub", Graphics::BlendOp::REV_SUBTRACT},
	{"Min", Graphics::BlendOp::MIN},
	{"Max", Graphics::BlendOp::MAX},
};

constexpr NamedValue<Graphics::ComparisonFunc> kZTests[] = {
	{"Less", Graphics::ComparisonFunc::LESS},
	{"Greater", Graphics::ComparisonFunc::GREATER},
	{"LEqual", Graphics::ComparisonFunc::LESS_EQUAL},
	{"GEqual", Graphics::ComparisonFunc::GREATER_EQUAL},
	{"Equal", Graphics::ComparisonFunc::EQUAL},
	{"NotEqual", Graphics::ComparisonFunc::NOT_EQUAL},
	{"Always", Graphics::ComparisonFunc::ALWAYS},
};

constexpr NamedValue<Graphics::CullMode> kCullModes[] = {
	{"Back", Graphics::CullMode::BACK},
	{"Front", Graphics::CullMode::FRONT},
	{"Off", Graphics::CullMode::NONE},
};

constexpr NamedValue<int> kRenderQueues[] = {
	{"Background", Shader::RenderQueue::BackGround},
	{"Geometry", Shader::RenderQueue::Geometry},
	{"AlphaTest", Shader::RenderQueue::AlphaTest},
	{"Transparent", Shader::RenderQueue::Transparent},
	{"Overlay", Shader::RenderQueue::Overlay},
};

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

class ShaderFormatError: public std::runtime_error {
public:
	using std::runtime_error::runtime_error;
};

template <typename T, size_t N>
T FindByName(const NamedValue<T> (&table)[N], const std::string &name, std::string_view key) {
	for (const auto &entry : table) {
		if (entry.name == name) { return entry.value; }
	}

	std::string names;
	for (const auto &entry : table) {
		if (!names.empty()) { names += ", "; }
		names += entry.name;
	}
	throw ShaderFormatError(std::format("\"{}\" に \"{}\" は使えません (使えるもの: {})", key, name, names));
}

std::optional<Json> LoadJson(const std::filesystem::path &path) {
	std::ifstream file(path);
	if (!file) {
		Log::Error("ファイルを開けませんでした: {}", ToUtf8(path));
		return std::nullopt;
	}

	try {
		return Json::parse(file, nullptr, true, true);
	} catch (const Json::parse_error &e) {
		Log::Error("json の書き方が間違っています: {}\n{}", ToUtf8(path), e.what());
		return std::nullopt;
	}
}

template <size_t N>
void WarnUnknownKeys(const Json &object, const std::string_view (&knownKeys)[N], const std::string &pathUtf8) {
	for (const auto &[key, value] : object.items()) {
		if (std::find(std::begin(knownKeys), std::end(knownKeys), key) == std::end(knownKeys)) {
			Log::Warning("無効なキーです (無視します): {}: \"{}\"", pathUtf8, key);
		}
	}
}

Graphics::Shader ImportStage(
	const std::filesystem::path &folder,
	const Json &pass,
	const char *key,
	Graphics::ShaderStage stage,
	bool required) {
	// キーが存在するか調べる
	const auto it = pass.find(key);
	if (it == pass.end()) {
		if (required) {
			throw ShaderFormatError(std::format("Pass に \"{}\" がありません", key));
		}
		return {};
	}

	// シェーダーアセットを作る
	const auto fileName = it->get<std::string>();
	const auto module = ShaderModuleImporter(folder / Utf8ToWide(fileName)).ImportShaderModule();
	if (!module) {
		throw ShaderFormatError(std::format("\"{}\" の HLSL をコンパイルできませんでした: {}", key, fileName));
	}

	// キーと同じステージか調べる
	if (module->GetStage() != stage) {
		throw ShaderFormatError(std::format("\"{}\" に書けるのは、このステージの HLSL だけです: {}", key, fileName));
	}

	return module->GetGraphicsShader();
}

Shader::Pass ParsePass(const std::filesystem::path &folder, const Json &json) {
	Shader::Pass pass{};
	pass.name = json.value("name", std::string());

	// シェーダー
	pass.vertexShader = ImportStage(folder, json, "vertex", Graphics::ShaderStage::VS, true);
	pass.pixelShader = ImportStage(folder, json, "pixel", Graphics::ShaderStage::PS, true);
	pass.hullShader = ImportStage(folder, json, "hull", Graphics::ShaderStage::HS, false);
	pass.domainShader = ImportStage(folder, json, "domain", Graphics::ShaderStage::DS, false);
	pass.geometryShader = ImportStage(folder, json, "geometry", Graphics::ShaderStage::GS, false);

	// ブレンド
	auto &target = pass.blend.renderTarget[0];
	target.srcBlend = FindByName(kBlendFactors, json.value("srcBlend", std::string("One")), "srcBlend");
	target.destBlend = FindByName(kBlendFactors, json.value("dstBlend", std::string("Zero")), "dstBlend");
	target.blendOp= FindByName(kBlendOps, json.value("blendOp", std::string("Add")), "blendOp");
	target.blendEnable = !(target.srcBlend == Graphics::Blend::ONE &&
						   target.destBlend == Graphics::Blend::ZERO &&
						   target.blendOp == Graphics::BlendOp::ADD);

	// 深度ステンシル
	pass.depthStencil.depthEnable = true;
	pass.depthStencil.depthWriteMask = json.value("zWrite", true) ? Graphics::DepthWriteMask::ALL
																  : Graphics::DepthWriteMask::ZERO;
	pass.depthStencil.comparisonFunc = FindByName(kZTests, json.value("zTest", std::string("LEqual")), "zTest");

	// ラスタライザ
	pass.rasterizer.cullMode = FindByName(kCullModes, json.value("cull", std::string("Back")), "cull");

	return pass;
}

} // namespace



ou::ShaderImporter::ShaderImporter(std::filesystem::path path)
	: AssetImporter(std::move(path)) {}


std::shared_ptr<Object> ou::ShaderImporter::Import() {
	return ImportShader();
}


std::shared_ptr<Shader> ou::ShaderImporter::ImportShader() {
	const auto &path = GetAssetPath();
	const auto pathUtf8 = ToUtf8(path);

	const auto json = LoadJson(path);
	if (!json) { return nullptr; }

	try {
		WarnUnknownKeys(*json, kShaderKeys, pathUtf8);

		// Render Queue
		auto renderQueue = Shader::RenderQueue::Geometry;
		if (const auto it = json->find("queue"); it != json->end()) {
			renderQueue = it->is_number_integer() ? it->get<int>()
												  : FindByName(kRenderQueues, it->get<std::string>(), "queue");
		}

		// Pass
		const auto &passesJson = json->at("passes");
		if (!passesJson.is_array() || passesJson.empty()) {
			throw ShaderFormatError("\"passes\" には Pass を 1 つ以上、配列で書いてください");
		}

		const auto folder = path.parent_path();
		std::vector<Shader::Pass> passes;
		for (const auto &passJson : passesJson) {
			if (!passJson.is_object()) {
				throw ShaderFormatError("\"passes\" の中身は {...} と書いてください");
			}
			WarnUnknownKeys(passJson, kPassKeys, pathUtf8);
			passes.push_back(ParsePass(folder, passJson));
		}

		auto shader = std::make_shared<Shader>(std::move(passes), renderQueue);
		shader->SetName(GetName());
		return shader;

	} catch (const std::exception &e) {
		Log::Error("シェーダの設定が間違っています: {}\n{}", pathUtf8, e.what());
		return nullptr;
	}
}
