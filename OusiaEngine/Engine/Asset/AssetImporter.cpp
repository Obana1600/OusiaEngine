#include "AssetImporter.h"
#include "Core/Paths.h"

#include <utility>

using namespace ou;


const std::string &ou::AssetImporter::GetAssetPath() const {
	return assetPath_;
}


const std::filesystem::path &ou::AssetImporter::GetFullPath() const {
	return fullPath_;
}


std::string ou::AssetImporter::ResolveAssetPath(std::string_view relativePath) const {
	const auto slash = assetPath_.find_last_of('/');
	const auto folder = (slash == std::string::npos) ? std::string()
													 : assetPath_.substr(0, slash + 1);
	return folder + std::string(relativePath);
}


ou::AssetImporter::AssetImporter(std::string assetPath, std::filesystem::path fullPath)
	: assetPath_(std::move(assetPath)),
	  fullPath_(std::move(fullPath)) {
	SetName(ToUtf8(fullPath_.filename()));
}
