#include "Color.h"
#include "Math/Mathf.h"

#include <format>

using namespace ou;


namespace {

// sRGB 変換用の定数
constexpr float kSrgbLinearThreshold = 0.04045f;
constexpr float kSrgbLinearScale = 12.92f;
constexpr float kSrgbOffset = 0.055f;
constexpr float kSrgbScale = 1.055f;
constexpr float kSrgbExponent = 2.4f;

// リニア変換用の定数
constexpr float kLinearSrgbThreshold = 0.0031308f;
constexpr float kLinearSrgbExponent = 1.0f / 2.4f;

// グレースケール用の定数
constexpr float kLuminanceR = 0.299f;
constexpr float kLuminanceG = 0.587f;
constexpr float kLuminanceB = 0.114f;


float SrgbToLinear(float c) {
	return (c <= kSrgbLinearThreshold)
			   ? c / kSrgbLinearScale
			   : Mathf::Pow((c + kSrgbOffset) / kSrgbScale, kSrgbExponent);
}

float LinearToSrgb(float c) {
	return (c <= kLinearSrgbThreshold)
			   ? c * kSrgbLinearScale
			   : kSrgbScale * Mathf::Pow(c, kLinearSrgbExponent) - kSrgbOffset;
}

} // namespace



const ou::Color ou::Color::White = {1.0f, 1.0f, 1.0f, 1.0f};
const ou::Color ou::Color::Black = {0.0f, 0.0f, 0.0f, 1.0f};
const ou::Color ou::Color::Clear = {0.0f, 0.0f, 0.0f, 0.0f};
const ou::Color ou::Color::Red = {1.0f, 0.0f, 0.0f, 1.0f};
const ou::Color ou::Color::Green = {0.0f, 1.0f, 0.0f, 1.0f};
const ou::Color ou::Color::Blue = {0.0f, 0.0f, 1.0f, 1.0f};
const ou::Color ou::Color::Yellow = {1.0f, 0.92f, 0.016f, 1.0f};
const ou::Color ou::Color::Cyan = {0.0f, 1.0f, 1.0f, 1.0f};
const ou::Color ou::Color::Magenta = {1.0f, 0.0f, 1.0f, 1.0f};
const ou::Color ou::Color::Gray = {0.5f, 0.5f, 0.5f, 1.0f};


ou::Color::Color(float r, float g, float b, float a)
	: r(r), g(g), b(b), a(a) {}


ou::Color::Color(const Vector3 &rgb, float a)
	: r(rgb.x),
	  g(rgb.y),
	  b(rgb.z),
	  a(a) {}


ou::Color::Color(const Vector4 &v)
	: r(v.x), g(v.y), b(v.z), a(v.w) {}


Color ou::Color::Gamma() const {
	return {LinearToSrgb(r), LinearToSrgb(g), LinearToSrgb(b), a};
}


Color ou::Color::GrayScale() const {
	const auto gray = kLuminanceR * r + kLuminanceG * g + kLuminanceB * b;
	return {gray, gray, gray, a};
}


Color ou::Color::Linear() const {
	return {SrgbToLinear(r), SrgbToLinear(g), SrgbToLinear(b), a};
}


float ou::Color::MaxColorComponent() const {
	return Mathf::Max(r, g, b);
}


float ou::Color::MinColorComponent() const {
	return Mathf::Min(r, g, b);
}


std::string ou::Color::ToString() const {
	return std::format("{} {} {} {}", r, g, b, a);
}


Color ou::Color::HSVToRGB(float h, float s, float v) {
	h = Mathf::Fmod(h, 360.0f);
	if (h < 0.0f) { h += 360.0f; }

	s = Mathf::Clamp01(s);
	v = Mathf::Clamp01(v);

	const auto c = v * s;
	const auto x = c * (1.0f - Mathf::Abs(Mathf::Fmod(h / 60.0f, 2.0f) - 1.0f));
	const auto m = v - c;

	auto r = 0.0f;
	auto g = 0.0f;
	auto b = 0.0f;

	if (h < 60.0f) {
		r = c;
		g = x;
	} else if (h < 120.0f) {
		r = x;
		g = c;
	} else if (h < 180.0f) {
		g = c;
		b = x;
	} else if (h < 240.0f) {
		g = x;
		b = c;
	} else if (h < 300.0f) {
		r = x;
		b = c;
	} else {
		r = c;
		b = x;
	}

	return {r + m, g + m, b + m, 1.0f};
}


Color ou::Color::Lerp(const Color &a, const Color &b, float t) {
	return a + (b - a) * t;
}


Color ou::Color::LerpUnclamped(const Color &a, const Color &b, float t) {
	t = Mathf::Clamp01(t);
	return Lerp(a, b, t);
}


void ou::Color::RGBToHSV(const Color &rgb, float &outH, float &outS, float &outV) {
	const auto maxC = rgb.MaxColorComponent();
	const auto minC = rgb.MinColorComponent();
	const auto delta = maxC - minC;

	// Value
	outV = maxC;

	// Saturation
	outS = (maxC == 0.0f) ? 0.0f
						  : delta / maxC;

	// Hue
	if (delta == 0.0f) {
		outH = 0.0f;
	} else if (maxC == rgb.r) {
		outH = 60.0f * Mathf::Fmod((rgb.g - rgb.b) / delta, 6.0f);
	} else if (maxC == rgb.g) {
		outH = 60.0f * ((rgb.b - rgb.r) / delta + 2.0f);
	} else {
		outH = 60.0f * ((rgb.r - rgb.g) / delta + 4.0f);
	}

	if (outH < 0.0f) { outH += 360.0f; }
}


namespace ou {

Color operator+(const Color &c1, const Color &c2) {
	return {c1.r + c2.r, c1.g + c2.g, c1.b + c2.b, c1.a + c2.a};
}


Color operator-(const Color &c1, const Color &c2) {
	return {c1.r - c2.r, c1.g - c2.g, c1.b - c2.b, c1.a - c2.a};
}


Color operator*(const Color &c1, const Color &c2) {
	return {c1.r * c2.r, c1.g * c2.g, c1.b * c2.b, c1.a * c2.a};
}


Color operator*(const Color &c1, float c2) {
	return {c1.r * c2, c1.g * c2, c1.b * c2, c1.a * c2};
}


Color operator*(float c1, const Color &c2) {
	return c2 * c1;
}


Color operator/(const Color &c1, float c2) {
	return {c1.r / c2, c1.g / c2, c1.b / c2, c1.a / c2};
}


bool operator==(const Color &c1, const Color &c2) {
	return Mathf::Approximately(c1.r, c2.r) &&
		   Mathf::Approximately(c1.g, c2.g) &&
		   Mathf::Approximately(c1.b, c2.b) &&
		   Mathf::Approximately(c1.a, c2.a);
}

} // namespace ou


Color &ou::Color::operator+=(const Color &c) {
	return *this = *this + c;
}


Color &ou::Color::operator-=(const Color &c) {
	return *this = *this - c;
}


Color &ou::Color::operator*=(const Color &c) {
	return *this = *this * c;
}


Color &ou::Color::operator*=(float c) {
	return *this = *this * c;
}


Color &ou::Color::operator/=(float c) {
	return *this = *this / c;
}


ou::Color::operator Vector3() const {
	return Vector3(r, g, b);
}


ou::Color::operator Vector4() const {
	return Vector4(r, g, b, a);
}
