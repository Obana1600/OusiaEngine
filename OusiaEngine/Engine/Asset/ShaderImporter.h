#pragma once

#include "Asset/AssetImporter.h"

#include <filesystem>
#include <memory>


namespace ou {

// 前方宣言
class Shader;

class ShaderImporter final: public AssetImporter {
public: // メンバ関数

	explicit ShaderImporter(std::filesystem::path path);

	std::shared_ptr<Object> Import() override;
	std::shared_ptr<Shader> ImportShader();
};

} // namespace ou
