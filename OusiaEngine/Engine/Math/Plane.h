#pragma once

#include "Math/Vector3.h"
#include <string>
#include <string_view>


namespace ou {

struct Plane {
public: // メンバ変数

	Vector3 normal;
	float distance;


public: // コンストラクタ

	Plane();

	// inNormal は正規化されたベクトルである必要がある
	Plane(const Vector3 &inNormal, const Vector3 &inPoint);
	Plane(const Vector3 &inNormal, float d);

	Plane(const Vector3 &a, const Vector3 &b, const Vector3 &c);


public: // メンバ関数

	Vector3 ClosestPointOnPlane(const Vector3 &point) const;

	float GetDistanceToPoint(const Vector3 &point) const;

	bool GetSide(const Vector3 &point) const;

	bool SameSide(const Vector3 &inPt0, const Vector3 &inPt1) const;

	bool Equals(const Plane &other) const;

	std::string ToString() const;
	std::string ToString(std::string_view format) const;


public: // オペレータ

	friend bool operator==(const Plane &lhs, const Plane &rhs) = default;
};

} // namespace ou
