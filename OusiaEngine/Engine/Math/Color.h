#pragma once

#include "Math/Vector3.h"
#include "Math/Vector4.h"

#include <string>


namespace ou {

struct Color {
public: // メンバ変数

	float r;
	float g;
	float b;
	float a;


public: // 静的メンバ変数

	static const Color White;
	static const Color Black;
	static const Color Clear;
	static const Color Red;
	static const Color Green;
	static const Color Blue;
	static const Color Yellow;
	static const Color Cyan;
	static const Color Magenta;
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

	operator Vector4() const;
};

} // namespace ou
