#pragma once

#include "Animation/Core/AnimationCurve.h"
#include "Animation/Core/QuaternionCurve.h"
#include "Math/Quaternion.h"
#include "Math/Vector3.h"

#include <array>
#include <string>


namespace ou {

struct BoneTransform {
	Vector3 position = Vector3::Zero;
	Quaternion rotation = Quaternion::Identity;
	Vector3 scale = Vector3::One;
};

struct BoneAnimationTrack {
public: // メンバ変数

	std::string path;

	std::array<AnimationCurve, 3> position;
	QuaternionCurve rotation;
	std::array<AnimationCurve, 3> scale;


public: // コンストラクタ
public: // メンバ関数

	BoneTransform Evaluate(float time, const BoneTransform &rest) const;
	float GetEndTime() const;
};

} // namespace ou
