#pragma once

#include <string>
#include <string_view>


namespace ou::EditorPayload {

constexpr const char *kGameObject = "OU_GAMEOBJECT";
constexpr const char *kAsset = "OU_ASSET";

struct AssetPayload {
	std::string path;
	std::string subAsset; // メインのアセットなら空
};

AssetPayload DecodeAsset(const void *data, int size);

std::string EncodeAsset(std::string_view assetPath, std::string_view subAsset);

} // namespace ou::EditorPayload
