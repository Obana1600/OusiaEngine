#pragma once

#include "Graphics/GraphicsTypes.h"

#include <vector>


namespace ou {

// 前方宣言
class Light;
struct PerFrame;


class AdditionalLightsShadowCasterPass final {
public: // メンバ関数

	AdditionalLightsShadowCasterPass() = delete;

	static void StaticInitialize();
	static void StaticFinalize();

	static void Render(const std::vector<const Light *> &lights, PerFrame &perFrame, Graphics::CommandList cmd);
};

} // namespace ou
