#pragma once

#include "Core/JsonFile.h"
#include "Graphics/GraphicsTypes.h"

#include <string>
#include <string_view>


// アセットの json に書く描画の設定の名前を変換する（読み書きは JsonFile）
// 名前が間違っていれば JsonFile::FormatError を投げる
namespace ou::AssetJson {

Graphics::Blend ToBlend(const std::string &name, std::string_view key);
Graphics::BlendOp ToBlendOp(const std::string &name, std::string_view key);
Graphics::ComparisonFunc ToComparisonFunc(const std::string &name, std::string_view key);
Graphics::CullMode ToCullMode(const std::string &name, std::string_view key);

int ToRenderQueue(const Json &value, std::string_view key);

} // namespace ou::AssetJson
