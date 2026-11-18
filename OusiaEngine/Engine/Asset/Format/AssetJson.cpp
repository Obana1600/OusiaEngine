#include "AssetJson.h"
#include "Render/Asset/Shader.h"

#include <format>

using namespace ou;
using namespace ou::AssetJson;


namespace {

// 名前と値の組
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

constexpr NamedValue<Graphics::ComparisonFunc> kComparisonFuncs[] = {
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
	throw JsonFile::FormatError(std::format("\"{}\" に \"{}\" は使えません (使えるもの: {})", key, name, names));
}

} // namespace



Graphics::Blend ou::AssetJson::ToBlend(const std::string &name, std::string_view key) {
	return FindByName(kBlendFactors, name, key);
}


Graphics::BlendOp ou::AssetJson::ToBlendOp(const std::string &name, std::string_view key) {
	return FindByName(kBlendOps, name, key);
}


Graphics::ComparisonFunc ou::AssetJson::ToComparisonFunc(const std::string &name, std::string_view key) {
	return FindByName(kComparisonFuncs, name, key);
}


Graphics::CullMode ou::AssetJson::ToCullMode(const std::string &name, std::string_view key) {
	return FindByName(kCullModes, name, key);
}


int ou::AssetJson::ToRenderQueue(const Json &value, std::string_view key) {
	if (value.is_number_integer()) { return value.get<int>(); }
	return FindByName(kRenderQueues, value.get<std::string>(), key);
}
