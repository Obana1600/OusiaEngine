#include "EditorPayload.h"

using namespace ou;


EditorPayload::AssetPayload ou::EditorPayload::DecodeAsset(const void *data, int size) {
	const std::string_view text(static_cast<const char *>(data), size > 0 ? static_cast<size_t>(size - 1) : 0);
	const auto bar = text.find('|');
	if (bar == std::string_view::npos) { return {std::string(text), {}}; }
	return {std::string(text.substr(0, bar)), std::string(text.substr(bar + 1))};
}


std::string ou::EditorPayload::EncodeAsset(std::string_view assetPath, std::string_view subAsset) {
	return std::string(assetPath) + "|" + std::string(subAsset);
}
