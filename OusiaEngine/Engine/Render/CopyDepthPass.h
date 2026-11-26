#pragma once

#include "Graphics/GraphicsTypes.h"


namespace ou {

// 前方宣言
class SceneRenderTarget;

class CopyDepthPass final {
public: // メンバ関数

	CopyDepthPass() = delete;

	static void StaticInitialize();
	static void StaticFinalize();

	static void Render(const SceneRenderTarget &target, Graphics::CommandList cmd);
};

} // namespace ou
