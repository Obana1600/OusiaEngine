#pragma once

#include "Graphics/GraphicsTypes.h"

#include <nlohmann/json.hpp>

#include <filesystem>
#include <optional>
#include <span>
#include <stdexcept>
#include <string>
#include <string_view>


namespace ou::AssetJson {

// json のエイリアス
using Json = nlohmann::json;

// 書き方の間違いを投げる
class FormatError: public std::runtime_error {
public:
	using std::runtime_error::runtime_error;
};

// コメント付きの json を読む
std::optional<Json> Load(const std::filesystem::path &path);

// KnownKeys にないキーを書き間違いとしてログに出力する
void WarnUnknownKeys(const Json &object, std::span<const std::string_view> knownKeys, const std::string &pathUtf8);

Graphics::Blend ToBlend(const std::string &name, std::string_view key);
Graphics::BlendOp ToBlendOp(const std::string &name, std::string_view key);
Graphics::ComparisonFunc ToComparisonFunc(const std::string &name, std::string_view key);
Graphics::CullMode ToCullMode(const std::string &name, std::string_view key);

int ToRenderQueue(const Json &value, std::string_view key);

} // namespace ou::AssetJson
