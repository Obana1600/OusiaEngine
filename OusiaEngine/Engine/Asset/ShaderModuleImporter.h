#pragma once

#include "Asset/AssetImporter.h"
#include "Graphics/GraphicsTypes.h"

#include <filesystem>
#include <memory>
#include <string>
#include <vector>


namespace ou {

// 前方宣言
class ShaderModule;

class ShaderModuleImporter final: public AssetImporter {
public: // メンバ関数

	explicit ShaderModuleImporter(std::filesystem::path assetPath);

	std::shared_ptr<Object> Import() override;
	std::shared_ptr<ShaderModule> ImportShaderModule();

	void SetStage(Graphics::ShaderStage stage);
	void SetMinShaderModel(Graphics::ShaderModel model);
	void SetDefines(std::vector<std::string> defines);


private: // メンバ変数

	Graphics::ShaderStage stage_ = Graphics::ShaderStage::Count;
	Graphics::ShaderModel minShaderModel_ = Graphics::ShaderModel::SM_6_6;
	std::vector<std::string> defines_;
};

} // namespace ou
