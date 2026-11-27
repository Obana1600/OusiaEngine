#pragma once

#include "Graphics/GraphicsTypes.h"
#include "Render/Camera.h"


namespace ou {

// 前方宣言
struct PerFrame;


class MainLightShadowCasterPass final {
public: // メンバ関数

	MainLightShadowCasterPass() = delete;

	static void StaticInitialize();
	static void StaticFinalize();

	static void Render(const Camera::PerCamera &camera, PerFrame &perFrame, Graphics::CommandList cmd);
};

} // namespace ou
