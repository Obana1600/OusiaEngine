#pragma once

#include "Math/Bounds.h"
#include "Math/Vector3.h"
#include "Physics/Collider.h"


namespace ou {

class SphereCollider: public Collider {
	friend class Physics;

public: // メンバ関数

	Vector3 ClosestPoint(const Vector3 &position) const override;
	Bounds GetBounds() const override;
	ColliderShape GetShape() const override;

	void SetCenter(const Vector3 &center);
	const Vector3 &GetCenter() const;

	void SetRadius(float radius);
	float GetRadius() const;


private: // メンバ変数

	Vector3 center_ = Vector3::Zero;
	float radius_ = 0.5f;


private: // メンバ関数

	Vector3 CalculateWorldCenter() const;
	float CalculateWorldRadius() const;
};

} // namespace ou
