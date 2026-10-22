#pragma once

#include "Math/Vector3.h"
#include <string>


namespace ou {

struct Bounds {
public: // メンバ変数

	Vector3 center;
	Vector3 extents;


public: // コンストラクタ

	Bounds();

	Bounds(const Vector3 &center, const Vector3 &size);


public: // メンバ関数

	void SetMinMax(const Vector3 &min, const Vector3 &max);
	void SetMin(const Vector3 &min);
	void SetMax(const Vector3 &max) ;
	Vector3 GetMin() const;
	Vector3 GetMax() const;

	void SetSize(const Vector3 &size);
	Vector3 GetSize() const;

	bool Contains(const Vector3 &point) const;
	bool Intersects(const Bounds &bounds) const;

	Vector3 ClosestPoint(const Vector3 &point) const;
	float SqrDistance(const Vector3 &point) const;

	void Encapsulate(const Vector3 &point);
	void Encapsulate(const Bounds &bounds);

	void Expand(float amount);

	std::string ToString() const;
};

} // namespace ou
