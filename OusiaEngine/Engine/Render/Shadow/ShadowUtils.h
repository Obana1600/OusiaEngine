#pragma once

#include "Math/Vector2.h"


namespace ou {

class ShadowUtils final {
	friend class MainLightShadowCasterPass;
	friend class AdditionalLightsShadowCasterPass;

public: // メンバ関数

	ShadowUtils() = delete;


private: // 静的メンバ変数

	// Soft のとき、ぼかしで広がる分だけ大きくするバイアス
	static constexpr float kSoftShadowBiasScale = 2.5f;

	// shadowDistance の最後の 20% で、影を薄くしていく
	static constexpr float kShadowFadeBorder = 0.2f;


private: // 静的メンバ関数

	static Vector2 CalculateShadowFadeParams(float shadowDistance);
};

} // namespace ou