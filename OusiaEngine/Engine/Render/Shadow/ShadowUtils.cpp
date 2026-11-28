#include "ShadowUtils.h"

using namespace ou;


Vector2 ou::ShadowUtils::CalculateShadowFadeParams(float shadowDistance) {
	const auto distanceSq = shadowDistance * shadowDistance;
	const auto border = (1.0f - kShadowFadeBorder) * (1.0f - kShadowFadeBorder);
	const auto fadeStartSq = border * distanceSq;

	const auto scale = 1.0f / (distanceSq - fadeStartSq);
	return Vector2(scale, -fadeStartSq * scale);
}
