#include "AssetImporter.h"
#include "Core/StringUtil.h"

#include <algorithm>
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


const std::vector<AssetImporter::SubAsset> &ou::AssetImporter::GetSubAssets() const {
	return subAssets_;
}


void ou::AssetImporter::ClearSubAssets() {
	subAssets_.clear();
}


ou::AssetImporter::AssetImporter(std::string assetPath, std::filesystem::path fullPath)
	: assetPath_(std::move(assetPath)),
	  fullPath_(std::move(fullPath)) {
	SetName(ToUtf8(fullPath_.filename()));
}


void ou::AssetImporter::AddSubAsset(std::string_view typeName, const std::shared_ptr<Object> &object) {
	if (!object) { return; }

	const auto &objectName = object->GetName();
	const auto base = std::string(typeName) + "/" + (objectName.empty() ? std::string(typeName) : objectName);

	const auto isUsed = [this](const std::string &name) {
		return std::any_of(subAssets_.begin(), subAssets_.end(),
						   [&](const SubAsset &sub) { return sub.name == name; });
	};

	auto name = base;
	for (int suffix = 1; isUsed(name); suffix++) {
		name = base + "_" + std::to_string(suffix);
	}

	subAssets_.push_back({std::move(name), object});
}
