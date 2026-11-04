#pragma once

#include "Core/Object.h"

#include <memory>


namespace ou {

enum class PhysicsMaterialCombine {
	Average,
	Minimum,
	Multiply,
	Maximum,
};

class PhysicsMaterial final: public Object {
public: // 静的メンバ関数

	static std::shared_ptr<PhysicsMaterial> Create(const PhysicsMaterial &source);


public: // メンバ関数

	void SetDynamicFriction(float dynamicFriction);
	float GetDynamicFriction() const;

	void SetStaticFriction(float staticFriction);
	float GetStaticFriction() const;

	void SetBounciness(float bounciness);
	float GetBounciness() const;

	void SetFrictionCombine(PhysicsMaterialCombine frictionCombine);
	PhysicsMaterialCombine GetFrictionCombine() const;

	void SetBounceCombine(PhysicsMaterialCombine bounceCombine);
	PhysicsMaterialCombine GetBounceCombine() const;


private: // メンバ変数

	float dynamicFriction_ = 0.6f;
	float staticFriction_ = 0.6f;
	float bounciness_ = 0.0f;
	PhysicsMaterialCombine frictionCombine_ = PhysicsMaterialCombine::Average;
	PhysicsMaterialCombine bounceCombine_ = PhysicsMaterialCombine::Average;
};

} // namespace ou
