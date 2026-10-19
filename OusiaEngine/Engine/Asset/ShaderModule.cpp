#include "ShaderModule.h"

#include <utility>

using namespace ou;


ou::ShaderModule::ShaderModule(Graphics::Shader shader)
	: graphicsShader_(std::move(shader)) {}


const Graphics::Shader &ou::ShaderModule::GetGraphicsShader() const {
	return graphicsShader_;
}


Graphics::ShaderStage ou::ShaderModule::GetStage() const {
	return graphicsShader_.stage;
}
