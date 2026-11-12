#include "AnimationCurve.h"
#include "Diagnostics/Log.h"
#include "Math/Mathf.h"

#include <algorithm>
#include <cmath>
#include <iterator>
#include <utility>

using namespace ou;


namespace {

// 接線の重みを使わないときの既定値
constexpr float kDefaultWeight = 1.0f / 3.0f;


bool HasInWeight(const Keyframe &key) {
	return key.weightedMode == WeightedMode::In || key.weightedMode == WeightedMode::Both;
}

bool HasOutWeight(const Keyframe &key) {
	return key.weightedMode == WeightedMode::Out || key.weightedMode == WeightedMode::Both;
}

// 3次ベジエ
float CubicBezier(float p0, float p1, float p2, float p3, float s) {
	const auto r = 1.0f - s;
	return (r * r * r * p0) + (3.0f * r * r * s * p1) + (3.0f * r * s * s * p2) + (s * s * s * p3);
}

// エルミート補間による評価
float EvaluateHermite(const Keyframe &left, const Keyframe &right, float duration, float t) {
	const auto t2 = t * t;
	const auto t3 = t2 * t;

	return (2.0f * t3 - 3.0f * t2 + 1.0f) * left.value +
		   (t3 - 2.0f * t2 + t) * duration * left.outTangent +
		   (-2.0f * t3 + 3.0f * t2) * right.value +
		   (t3 - t2) * duration * right.inTangent;
}

// 3次ベジエによる評価
float EvaluateBezier(const Keyframe &left, const Keyframe &right, float duration, float t) {
	const auto outWeight = HasOutWeight(left) ? Mathf::Clamp01(left.outWeight) : kDefaultWeight;
	const auto inWeight = HasInWeight(right) ? Mathf::Clamp01(right.inWeight) : kDefaultWeight;

	auto low = 0.0f;
	auto high = 1.0f;
	for (int i = 0; i < 24; i++) {
		const auto s = (low + high) * 0.5f;
		if (CubicBezier(0.0f, outWeight, 1.0f - inWeight, 1.0f, s) < t) {
			low = s;
		} else {
			high = s;
		}
	}
	const auto s = (low + high) * 0.5f;

	const auto p1 = left.value + left.outTangent * duration * outWeight;
	const auto p2 = right.value - right.inTangent * duration * inWeight;
	return CubicBezier(left.value, p1, p2, right.value, s);
}

} // namespace



AnimationCurve ou::AnimationCurve::Constant(float timeStart, float timeEnd, float value) {
	return Linear(timeStart, value, timeEnd, value);
}


AnimationCurve ou::AnimationCurve::Linear(float timeStart, float valueStart, float timeEnd, float valueEnd) {
	if (timeStart == timeEnd) {
		return AnimationCurve({Keyframe(timeStart, valueStart)});
	}

	const auto tangent = (valueEnd - valueStart) / (timeEnd - timeStart);
	return AnimationCurve({Keyframe(timeStart, valueStart, tangent, tangent),
						   Keyframe(timeEnd, valueEnd, tangent, tangent)});
}


AnimationCurve ou::AnimationCurve::EaseInOut(float timeStart, float valueStart, float timeEnd, float valueEnd) {
	if (timeStart == timeEnd) {
		return AnimationCurve({Keyframe(timeStart, valueStart)});
	}

	return AnimationCurve({Keyframe(timeStart, valueStart, 0.0f, 0.0f),
						   Keyframe(timeEnd, valueEnd, 0.0f, 0.0f)});
}


ou::AnimationCurve::AnimationCurve(std::vector<Keyframe> keys) {
	SetKeys(std::move(keys));
}


float ou::AnimationCurve::Evaluate(float time) const {
	if (keys_.empty()) { return 0.0f; }
	if (time <= keys_.front().time) { return keys_.front().value; }
	if (time >= keys_.back().time) { return keys_.back().value; }

	const auto next = std::upper_bound(keys_.begin(), keys_.end(), time,
									   [](float t, const Keyframe &key) { return t < key.time; });
	const auto &k0 = *std::prev(next);
	const auto &k1 = *next;

	// 接線が無限大の区間は補間しない
	if (std::isinf(k0.outTangent) || std::isinf(k1.inTangent)) { return k0.value; }

	const auto duration = k1.time - k0.time;
	const auto t = (time - k0.time) / duration;

	if (HasOutWeight(k0) || HasInWeight(k1)) {
		return EvaluateBezier(k0, k1, duration, t);
	}
	return EvaluateHermite(k0, k1, duration, t);
}


int ou::AnimationCurve::AddKey(float time, float value) {
	return AddKey(Keyframe(time, value));
}


int ou::AnimationCurve::AddKey(const Keyframe &key) {
	const auto isSameTime = [&](const Keyframe &k) { return k.time == key.time; };
	if (std::any_of(keys_.begin(), keys_.end(), isSameTime)) { return -1; }

	return InsertKey(key);
}


int ou::AnimationCurve::MoveKey(int index, const Keyframe &key) {
	if (!IsValidIndex(index)) {
		Log::Error("AnimationCurve::MoveKey の番号が範囲外です: {} (キーの数 = {})", index, keys_.size());
		return -1;
	}

	auto moved = key;
	const auto oldTime = keys_[index].time;
	keys_.erase(keys_.begin() + index);

	const auto isSameTime = [&](const Keyframe &k) { return k.time == moved.time; };
	if (std::any_of(keys_.begin(), keys_.end(), isSameTime)) {
		moved.time = oldTime;
	}

	return InsertKey(moved);
}


void ou::AnimationCurve::RemoveKey(int index) {
	if (!IsValidIndex(index)) {
		Log::Error("AnimationCurve::RemoveKey の番号が範囲外です: {} (キーの数 = {})", index, keys_.size());
		return;
	}

	keys_.erase(keys_.begin() + index);
}


void ou::AnimationCurve::SetKeys(std::vector<Keyframe> keys) {
	keys_ = std::move(keys);
	std::stable_sort(keys_.begin(), keys_.end(),
					 [](const Keyframe &a, const Keyframe &b) { return a.time < b.time; });
}


const std::vector<Keyframe> &ou::AnimationCurve::GetKeys() const {
	return keys_;
}


int ou::AnimationCurve::GetLength() const {
	return static_cast<int>(keys_.size());
}


const Keyframe &ou::AnimationCurve::operator[](int index) const {
	static const Keyframe kNull;
	if (!IsValidIndex(index)) {
		Log::Error("AnimationCurve::operator[] の番号が範囲外です: {} (キーの数 = {})", index, keys_.size());
		return kNull;
	}

	return keys_[index];
}


int ou::AnimationCurve::InsertKey(const Keyframe &key) {
	const auto it = std::lower_bound(keys_.begin(), keys_.end(), key.time,
									 [](const Keyframe &k, float t) { return k.time < t; });
	return static_cast<int>(keys_.insert(it, key) - keys_.begin());
}


bool ou::AnimationCurve::IsValidIndex(int index) const {
	return index >= 0 && index < GetLength();
}
