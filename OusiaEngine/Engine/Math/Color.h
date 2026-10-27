#pragma once

#include "Math/Vector3.h"
#include "Math/Vector4.h"

#include <string>
#include <string_view>


namespace ou {

struct Color {
public: // メンバ変数

	float r;
	float g;
	float b;
	float a;


public: // 静的メンバ変数

	// Color(1, 1, 1, 1)
	static const Color White;

	// Color(0, 0, 0, 1)
	static const Color Black;

	// Color(0, 0, 0, 0)
	static const Color Clear;

	// Color(1, 0, 0, 1)
	static const Color Red;

	// Color(0, 1, 0, 1)
	static const Color Green;

	// Color(0, 0, 1, 1)
	static const Color Blue;

	// Color(1, 0.92, 0.016, 1)
	static const Color Yellow;

	// Color(0, 1, 1, 1)
	static const Color Cyan;

	// Color(1, 0, 1, 1)
	static const Color Magenta;

	// Color(0.5, 0.5, 0, 1)
	static const Color Gray;


public: // コンストラクタ

	Color() = default;

	Color(float r, float g, float b, float a);

	Color(const Vector3 &rgb, float a);

	explicit Color(const Vector4 &v);


public: // メンバ関数

	Color Gamma() const;

	Color GrayScale() const;

	Color Linear() const;

	float MaxColorComponent() const;
	float MinColorComponent() const;

	std::string ToString() const;
	std::string ToString(std::string_view format) const;


public: // 静的メンバ関数

	static Color HSVToRGB(float h, float s, float v);

	static Color Lerp(const Color &a, const Color &b, float t);

	static Color LerpUnclamped(const Color &a, const Color &b, float t);

	static void RGBToHSV(const Color &rgb, float &outH, float &outS, float &outV);


public: // オペレータ

	friend Color operator+(const Color &c1, const Color &c2);
	friend Color operator-(const Color &c1, const Color &c2);
	friend Color operator*(const Color &c1, const Color &c2);
	friend Color operator*(const Color &c1, float c2);
	friend Color operator*(float c1, const Color &c2);
	friend Color operator/(const Color &c1, float c2);
	friend bool operator==(const Color &c1, const Color &c2);

	Color &operator+=(const Color &c);
	Color &operator-=(const Color &c);
	Color &operator*=(const Color &c);
	Color &operator*=(float c);
	Color &operator/=(float c);

	operator Vector3() const;
	operator Vector4() const;
};

} // namespace ou
