#pragma once

#include "Math/Vector2.h"

#include <string>
#include <string_view>


namespace ou {

struct Rect {
public: // メンバ変数

	float x;
	float y;
	float width;
	float height;


public: // 定数

	// Rect(0, 0, 0, 0)
	static const Rect Zero;


public: // コンストラクタ

	Rect();

	Rect(float x, float y, float width, float height);

	Rect(const Vector2 &position, const Vector2 &size);


public: // メンバ関数

	void SetPosition(const Vector2 &position);
	Vector2 GetPosition() const;

	void SetSize(const Vector2 &size);
	Vector2 GetSize() const;

	void SetCenter(const Vector2 &center);
	Vector2 GetCenter() const;

	void SetMin(const Vector2 &min);
	Vector2 GetMin() const;

	void SetMax(const Vector2 &max);
	Vector2 GetMax() const;

	void SetXMin(float xMin);
	float GetXMin() const;

	void SetYMin(float yMin);
	float GetYMin() const;

	void SetXMax(float xMax);
	float GetXMax() const;

	void SetYMax(float yMax);
	float GetYMax() const;

	bool Contains(const Vector2 &point) const;
	bool Overlaps(const Rect &other) const;

	std::string ToString() const;
	std::string ToString(std::string_view format) const;


public: // 静的メンバ関数

	static Rect MinMaxRect(float xMin, float yMin, float xMax, float yMax);

	static Vector2 NormalizedToPoint(const Rect &rect, const Vector2 &normalizedCoordinates);

	static Vector2 PointToNormalized(const Rect &rect, const Vector2 &point);


public: // オペレータ

	friend bool operator==(const Rect &a, const Rect &b);
};

} // namespace ou
