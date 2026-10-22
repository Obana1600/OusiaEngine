#pragma once

#include "Math/Bounds.h"
#include "Math/Vector3.h"
#include "Physics/Collider.h"


namespace ou {

class BoxCollider: public Collider {
	friend class Physics;

public: // メンバ関数

	Vector3 ClosestPoint(const Vector3 &position) const override;
	Bounds GetBounds() const override;
	ColliderShape GetShape() const override;

	void SetCenter(const Vector3 &center);
	const Vector3 &GetCenter() const;

	void SetSize(const Vector3 &size);
	const Vector3 &GetSize() const;


private: // インナークラス

	struct WorldBox {
		Vector3 center;
		Vector3 axes[3];
		Vector3 halfExtents;
	};


private: // メンバ変数

	Vector3 center_ = Vector3::Zero;
	Vector3 size_ = Vector3::One;


private: // メンバ関数

	WorldBox CalculateWorldBox() const;
};

} // namespace ou
