#pragma once

#include "Math/Bounds.h"
#include "Math/Vector3.h"
#include "Scene/Behaviour.h"


namespace ou {

enum class ColliderShape {
	Box,
	Sphere
};

class Collider: public Behaviour {
public: // メンバ関数

	virtual Vector3 ClosestPoint(const Vector3 &position) const = 0;
	Vector3 ClosestPointOnBounds(const Vector3 &position) const;

	virtual Bounds GetBounds() const = 0;
	virtual ColliderShape GetShape() const = 0;

	void SetIsTrigger(bool value);
	bool IsTrigger() const;


protected: // メンバ関数

	void OnEnable() override;
	void OnDisable() override;


private: // メンバ変数

	bool isTrigger_ = false;
};

} // namespace ou
