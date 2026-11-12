#pragma once

#include "Math/Quaternion.h"
#include "Math/Vector4.h"

#include <vector>


// TODO: FBX の重み付きベジエにも対応させる

namespace ou {

enum class QuaternionInterpolation {
	Linear,
	Step,
	CubicHermite,
};

struct QuaternionKeyframe {
	float time = 0.0f;
	Quaternion value = Quaternion::Identity;
	Vector4 inTangent = Vector4::Zero;
	Vector4 outTangent = Vector4::Zero;
};

class QuaternionCurve {
public: // コンストラクタ

	QuaternionCurve() = default;
	QuaternionCurve(std::vector<QuaternionKeyframe> keys, QuaternionInterpolation interpolation);


public: // メンバ関数

	Quaternion Evaluate(float time) const;

	void SetKeys(std::vector<QuaternionKeyframe> keys);
	const std::vector<QuaternionKeyframe> &GetKeys() const;

	void SetInterpolation(QuaternionInterpolation interpolation);
	QuaternionInterpolation GetInterpolation() const;

	int GetLength() const;


public: // オペレータ

	//const QuaternionKeyframe &operator[](int index) const;


private: // メンバ変数

	std::vector<QuaternionKeyframe> keys_;
	QuaternionInterpolation interpolation_ = QuaternionInterpolation::Linear;
};

} // namespace ou
