#include "QuaternionCurve.h"

#include <algorithm>
#include <iterator>
#include <utility>

using namespace ou;


namespace {

Quaternion EvaluateCubicSpline(const QuaternionKeyframe &k0, const QuaternionKeyframe &k1, float t, float duration) {
	const auto t2 = t * t;
	const auto t3 = t2 * t;

	const auto h00 = 2.0f * t3 - 3.0f * t2 + 1.0f;
	const auto h10 = (t3 - 2.0f * t2 + t) * duration;
	const auto h01 = -2.0f * t3 + 3.0f * t2;
	const auto h11 = (t3 - t2) * duration;

	Quaternion result;
	for (int i = 0; i < 4; i++) {
		result[i] = h00 * k0.value[i] + h10 * k0.outTangent[i] + h01 * k1.value[i] + h11 * k1.inTangent[i];
	}
	return result.Normalized();
}

} // namespace



ou::QuaternionCurve::QuaternionCurve(std::vector<QuaternionKeyframe> keys, QuaternionInterpolation interpolation)
	: interpolation_(interpolation) {
	SetKeys(std::move(keys));
}


Quaternion ou::QuaternionCurve::Evaluate(float time) const {
	if (keys_.empty()) { return Quaternion::Identity; }
	if (time <= keys_.front().time) { return keys_.front().value; }
	if (time >= keys_.back().time) { return keys_.back().value; }

	const auto next = std::upper_bound(keys_.begin(), keys_.end(), time,
									   [](float t, const QuaternionKeyframe &key) { return t < key.time; });
	const auto &k0 = *std::prev(next);
	const auto &k1 = *next;

	const auto duration = k1.time - k0.time;
	const auto t = (time - k0.time) / duration;

	switch (interpolation_) {
		default:
		case QuaternionInterpolation::Linear:
			return Quaternion::Slerp(k0.value, k1.value, t);
		case QuaternionInterpolation::Step:
			return k0.value;
		case QuaternionInterpolation::CubicHermite:
			return EvaluateCubicSpline(k0, k1, t, duration);
	}
}


void ou::QuaternionCurve::SetKeys(std::vector<QuaternionKeyframe> keys) {
	keys_ = std::move(keys);
	std::stable_sort(keys_.begin(), keys_.end(),
					 [](const QuaternionKeyframe &a, const QuaternionKeyframe &b) { return a.time < b.time; });
}


const std::vector<QuaternionKeyframe> &ou::QuaternionCurve::GetKeys() const {
	return keys_;
}


void ou::QuaternionCurve::SetInterpolation(QuaternionInterpolation interpolation) {
	interpolation_ = interpolation;
}


QuaternionInterpolation ou::QuaternionCurve::GetInterpolation() const {
	return interpolation_;
}


int ou::QuaternionCurve::GetLength() const {
	return static_cast<int>(keys_.size());
}


//const QuaternionKeyframe &ou::QuaternionCurve::operator[](int index) const {
//	static const QuaternionKeyframe kNull;
//	if (!IsValidIndex(index)) {
//		Log::Error("AnimationCurve::operator[] の番号が範囲外です: {} (キーの数 = {})", index, keys_.size());
//		return kNull;
//	}
//
//	return keys_[index];
//}
