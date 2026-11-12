#pragma once

#include "Animation/Core/WrapMode.h"
#include "Animation/Skeletal/BoneAnimationTrack.h"
#include "Core/Object.h"

#include <vector>


namespace ou {

class SkeletalAnimationClip: public Object {
public: // メンバ関数

	void AddTrack(BoneAnimationTrack track);
	const std::vector<BoneAnimationTrack> &GetTracks() const;

	void SetWrapMode(WrapMode wrapMode);
	WrapMode GetWrapMode() const;

	float GetLength() const;


private: // メンバ変数

	std::vector<BoneAnimationTrack> tracks_;
	WrapMode wrapMode_ = WrapMode::Default;
	float length_ = 0.0f;
};

} // namespace ou
