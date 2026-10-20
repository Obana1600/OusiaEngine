#pragma once

#include "Graphics/GraphicsTypes.h"
#include "Scene/Object.h"


namespace ou {

class ShaderModule final: public Object {
public: // メンバ関数

	explicit ShaderModule(Graphics::Shader shader);

	const Graphics::Shader &GetGraphicsShader() const;
	Graphics::ShaderStage GetStage() const;


private: // メンバ変数

	Graphics::Shader graphicsShader_;
};

} // namespace ou
