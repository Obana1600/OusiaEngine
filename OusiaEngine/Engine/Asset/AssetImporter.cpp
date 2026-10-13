#include "AssetImporter.h"
#include "Core/Paths.h"

#include <utility>

using namespace ou;


const std::filesystem::path &ou::AssetImporter::GetAssetPath() const {
	return assetPath_;
}


ou::AssetImporter::AssetImporter(std::filesystem::path assetPath)
	: assetPath_(std::move(assetPath)) {
	SetName(ToUtf8(assetPath_.filename()));
}
