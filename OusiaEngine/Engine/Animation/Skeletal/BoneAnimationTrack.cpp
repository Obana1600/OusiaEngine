#include "BoneAnimationTrack.h"
#include "Math/Mathf.h"

using namespace ou;


BoneTransform ou::BoneAnimationTrack::Evaluate(float time, const BoneTransform &rest) const {
	auto result = rest;

	for (int i = 0; i < 3; i++) {
		if (position[i].GetLength() > 0) { result.position[i] = position[i].Evaluate(time); }
		if (rotation.GetLength() > 0) { result.rotation = rotation.Evaluate(time); }
		if (scale[i].GetLength() > 0) { result.scale[i] = scale[i].Evaluate(time); }
	}

	return result;
}


float ou::BoneAnimationTrack::GetEndTime() const {
	auto endTime = 0.0f;
	const auto extend = [&](const auto &keys) { 
		if (!keys.empty()){endTime = Mathf::Max(endTime, keys.back().time);} };

	for (const auto &curve : position) {
		extend(curve.GetKeys());
	}
	extend(rotation.GetKeys());
	for (const auto &curve : scale) {
		extend(curve.GetKeys());
	}

	return endTime;
}
