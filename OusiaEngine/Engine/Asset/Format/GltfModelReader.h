#pragma once

#include "Asset/Format/ModelData.h"

#include <filesystem>
#include <optional>


namespace ou {

std::optional<ModelData> ReadGltfModel(const std::filesystem::path &path);

} // namespace ou
