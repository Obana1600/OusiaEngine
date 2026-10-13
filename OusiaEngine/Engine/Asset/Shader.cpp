#include "Shader.h"

#include <utility>

using namespace ou;


ou::Shader::Shader(Graphics::Shader shader)
	: graphicsShader_(std::move(shader)) {}


const Graphics::Shader &ou::Shader::GetGraphicsShader() const {
	return graphicsShader_;
}


Graphics::ShaderStage ou::Shader::GetStage() const {
	return graphicsShader_.stage;
}
