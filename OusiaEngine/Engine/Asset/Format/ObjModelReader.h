#pragma once

#include "Asset/Format/ModelData.h"

#include <filesystem>
#include <optional>


namespace ou {

std::optional<ModelData> ReadObjModel(const std::filesystem::path &path);

} // namespace ou
