#pragma once

#include "Graphics/GraphicsTypes.h"


namespace ou {

// 前方宣言
class SceneRenderTarget;
class VolumeStack;

class PostProcessPass final {
public: // メンバ関数

	PostProcessPass() = delete;

	static void StaticInitialize();
	static void StaticFinalize();

	static void Render(const SceneRenderTarget &target, const VolumeStack *stack, Graphics::CommandList cmd);
};

} // namespace ou
