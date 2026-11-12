#include "Easing.h"
#include "Math/Mathf.h"

using namespace ou;


namespace {

// sine -------------------------------------------------------------------------------------------

float InSine(float t) {
	return 1.0f - Mathf::Cos((t * Mathf::PI) / 2.0f);
}

float OutSine(float t) {
	return Mathf::Sin((t * Mathf::PI) / 2.0f);
}

float InOutSine(float t) {
	return -(Mathf::Cos(Mathf::PI * t) - 1.0f) / 2.0f;
}

// ------------------------------------------------------------------------------------------------


// quad ---------------------------------------------------------------------------------------

float InQuad(float t) {
	return t * t;
}

float OutQuad(float t) {
	return 1.0f - (1.0f - t) * (1.0f - t);
}

float InOutQuad(float t) {
	const auto n = -2.0f * t + 2.0f;
	return (t < 0.5f) ? 2.0f * t * t
					  : 1.0f - (n * n) / 2.0f;
}

// --------------------------------------------------------------------------------------------


// cubic --------------------------------------------------------------------------------------

float InCubic(float t) {
	return t * t * t;
}

float OutCubic(float t) {
	const auto n = 1.0f - t;
	return 1.0f - n * n * n;
}

float InOutCubic(float t) {
	const auto n = -2.0f * t + 2.0f;
	return (t < 0.5f) ? 4.0f * t * t * t
					  : 1.0f - (n * n * n) / 2.0f;
}

// --------------------------------------------------------------------------------------------


// quart --------------------------------------------------------------------------------------

float InQuart(float t) {
	return t * t * t * t;
}

float OutQuart(float t) {
	const auto n = 1.0f - t;
	return 1.0f - n * n * n * n;
}

float InOutQuart(float t) {
	if (t < 0.5f) {
		return 8.0f * t * t * t * t;
	} else {
		const auto n = -2.0f * t + 2.0f;
		return 1.0f - (n * n * n * n) / 2.0f;
	}
}

// --------------------------------------------------------------------------------------------


// quint --------------------------------------------------------------------------------------

float InQuint(float t) {
	return t * t * t * t * t;
}

float OutQuint(float t) {
	const auto n = 1.0f - t;
	return 1.0f - n * n * n * n * n;
}

float InOutQuint(float t) {
	const auto n = -2.0f * t + 2.0f;
	return (t < 0.5f) ? 16.0f * t * t * t * t * t
					  : 1.0f - (n * n * n * n * n) / 2.0f;
}

// --------------------------------------------------------------------------------------------


// expo ---------------------------------------------------------------------------------------

float InExpo(float t) {
	return (t <= 0.0f) ? 0.0f
					   : Mathf::Pow(2.0f, 10.0f * t - 10.0f);
}

float OutExpo(float t) {
	return (t >= 1.0f) ? 1.0f
					   : 1.0f - Mathf::Pow(2.0f, -10.0f * t);
}

float InOutExpo(float t) {
	return (t <= 0.0f)	 ? 0.0f
		   : (t >= 1.0f) ? 1.0f
		   : (t < 0.5f)	 ? Mathf::Pow(2.0f, 20.0f * t - 10.0f) / 2.0f
						 : (2.0f - Mathf::Pow(2.0f, -20.0f * t + 10.0f)) / 2.0f;
}

// --------------------------------------------------------------------------------------------


// circ ---------------------------------------------------------------------------------------

float InCirc(float t) {
	return 1.0f - Mathf::Sqrt(1.0f - t * t);
}

float OutCirc(float t) {
	const auto n = t - 1.0f;
	return Mathf::Sqrt(1.0f - n * n);
}

float InOutCirc(float t) {
	const auto a = 2.0f * t;
	const auto b = -2.0f * t + 2.0f;
	return (t < 0.5f) ? (1.0f - Mathf::Sqrt(1.0f - a * a)) / 2.0f
					  : (Mathf::Sqrt(1.0f - b * b) + 1.0f) / 2.0f;
}

// --------------------------------------------------------------------------------------------


// back ---------------------------------------------------------------------------------------

float InBack(float t) {
	constexpr auto c1 = 1.70158f;
	constexpr auto c3 = c1 + 1.0f;

	return c3 * t * t * t - c1 * t * t;
}

float OutBack(float t) {
	constexpr auto c1 = 1.70158f;
	constexpr auto c3 = c1 + 1.0f;

	const auto n = t - 1.0f;
	return 1.0f + c3 * n * n * n + c1 * n * n;
}

float InOutBack(float t) {
	constexpr auto c1 = 1.70158f;
	constexpr auto c2 = c1 * 1.525f;

	const auto a = 2.0f * t;
	const auto b = 2.0f * t - 2.0f;
	return (t < 0.5f) ? (a * a * ((c2 + 1.0f) * a - c2)) / 2.0f
					  : (b * b * ((c2 + 1.0f) * b + c2) + 2.0f) / 2.0f;
}

// --------------------------------------------------------------------------------------------


// elastic ------------------------------------------------------------------------------------

float InElastic(float t) {
	constexpr auto c4 = (2.0f * Mathf::PI) / 3.0f;

	return (t <= 0.0f)	 ? 0.0f
		   : (t >= 1.0f) ? 1.0f
						 : -Mathf::Pow(2.0f, 10.0f * t - 10.0f) * Mathf::Sin((t * 10.0f - 10.75f) * c4);
}

float OutElastic(float t) {
	constexpr auto c4 = (2.0f * Mathf::PI) / 3.0f;

	return (t <= 0.0f)	 ? 0.0f
		   : (t >= 1.0f) ? 1.0f
						 : Mathf::Pow(2.0f, -10.0f * t) * Mathf::Sin((t * 10.0f - 0.75f) * c4) + 1.0f;
}

float InOutElastic(float t) {
	constexpr auto c5 = (2.0f * Mathf::PI) / 4.5f;

	return (t <= 0.0f)	 ? 0.0f
		   : (t >= 1.0f) ? 1.0f
		   : (t < 0.5f)	 ? -(Mathf::Pow(2.0f, 20.0f * t - 10.0f) * Mathf::Sin((20.0f * t - 11.125f) * c5)) / 2.0f
						 : (Mathf::Pow(2.0f, -20.0f * t + 10.0f) * Mathf::Sin((20.0f * t - 11.125f) * c5)) / 2.0f + 1.0f;
}

// --------------------------------------------------------------------------------------------


// bounce -------------------------------------------------------------------------------------

float OutBounce(float t) {
	constexpr auto n1 = 7.5625f;
	constexpr auto d1 = 2.75f;

	if (t < 1.0f / d1) {
		return n1 * t * t;
	} else if (t < 2.0f / d1) {
		const auto u = t - 1.5f / d1;
		return n1 * u * u + 0.75f;
	} else if (t < 2.5f / d1) {
		const auto u = t - 2.25f / d1;
		return n1 * u * u + 0.9375f;
	} else {
		const auto u = t - 2.625f / d1;
		return n1 * u * u + 0.984375f;
	}
}

float InBounce(float t) {
	return 1.0f - OutBounce(1.0f - t);
}

float InOutBounce(float t) {
	return (t < 0.5f) ? (1.0f - OutBounce(1.0f - 2.0f * t)) / 2.0f
					  : (1.0f + OutBounce(2.0f * t - 1.0f)) / 2.0f;
}

// --------------------------------------------------------------------------------------------

} // namespace



float ou::Easing::Evaluate(EaseType type, float t) {
	t = Mathf::Clamp01(t);

	switch (type) {
		case EaseType::Linear:
			return t;

		case EaseType::InSine:
			return InSine(t);
		case EaseType::OutSine:
			return OutSine(t);
		case EaseType::InOutSine:
			return InOutSine(t);

		case EaseType::InQuad:
			return InQuad(t);
		case EaseType::OutQuad:
			return OutQuad( t);
		case EaseType::InOutQuad:
			return InOutQuad(t);

		case EaseType::InCubic:
			return InCubic(t);
		case EaseType::OutCubic:
			return OutCubic(t);
		case EaseType::InOutCubic:
			return InOutCubic(t);

		case EaseType::InQuart:
			return InQuart(t);
		case EaseType::OutQuart:
			return OutQuart(t);
		case EaseType::InOutQuart:
			return InOutQuart(t);

		case EaseType::InQuint:
			return InQuint(t);
		case EaseType::OutQuint:
			return OutQuint(t);
		case EaseType::InOutQuint:
			return InOutQuint(t);

		case EaseType::InExpo:
			return InExpo(t);
		case EaseType::OutExpo:
			return OutExpo(t);
		case EaseType::InOutExpo:
			return InOutExpo(t);

		case EaseType::InCirc:
			return InCirc(t);
		case EaseType::OutCirc:
			return OutCirc(t);
		case EaseType::InOutCirc:
			return InOutCirc(t);

		case EaseType::InBack:
			return InBack(t);
		case EaseType::OutBack:
			return OutBack(t);
		case EaseType::InOutBack:
			return InOutBack(t);

		case EaseType::InElastic:
			return InElastic(t);
		case EaseType::OutElastic:
			return OutElastic(t);
		case EaseType::InOutElastic:
			return InOutElastic(t);

		case EaseType::InBounce:
			return InBounce(t);
		case EaseType::OutBounce:
			return OutBounce(t);
		case EaseType::InOutBounce:
			return InOutBounce(t);
	}

	return t;
}
