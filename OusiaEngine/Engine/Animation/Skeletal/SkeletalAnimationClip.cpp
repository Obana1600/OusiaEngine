#include "SkeletalAnimationClip.h"
#include "Math/Mathf.h"

#include <utility>

using namespace ou;


void ou::SkeletalAnimationClip::AddTrack(BoneAnimationTrack track) {
	length_ = Mathf::Max(length_, track.GetEndTime());
	tracks_.push_back(std::move(track));
}


const std::vector<BoneAnimationTrack> &ou::SkeletalAnimationClip::GetTracks() const {
	return tracks_;
}


void ou::SkeletalAnimationClip::SetWrapMode(WrapMode wrapMode) {
	wrapMode_ = wrapMode;
}


WrapMode ou::SkeletalAnimationClip::GetWrapMode() const {
	return wrapMode_;
}


float ou::SkeletalAnimationClip::GetLength() const {
	return length_;
}
