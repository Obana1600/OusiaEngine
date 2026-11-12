#include "SkeletalAnimator.h"
#include "Animation/Skeletal/SkeletalAnimationClip.h"
#include "Diagnostics/Log.h"
#include "Math/Mathf.h"
#include "Scene/Scene.h"
#include "Scene/SceneManager.h"
#include "Scene/Transform.h"
#include "Serialize/Serializer.h"

#include <unordered_set>
#include <utility>

using namespace ou;


namespace {

BoneTransform ReadTransform(const Transform &transform) {
	return {transform.GetLocalPosition(),
			transform.GetLocalRotation(),
			transform.GetLocalScale()};
}

void WriteTransform(Transform &transform, const BoneTransform &value) {
	transform.SetLocalPosition(value.position);
	transform.SetLocalRotation(value.rotation);
	transform.SetLocalScale(value.scale);
}

BoneTransform BlendBone(const BoneTransform &from, const BoneTransform &to, float weight) {
	return {Vector3::Lerp(from.position, to.position, weight),
			Quaternion::Slerp(from.rotation, to.rotation, weight),
			Vector3::Lerp(from.scale, to.scale, weight)};
}

} // namespace



void ou::SkeletalAnimator::Awake() {
	SceneManager::RegisterSkeletalAnimator(this);
}


void ou::SkeletalAnimator::Start() {
	if (playAutomatically_ && clip_ && !isPlaying_) {
		Play(clip_);
	}
}


void ou::SkeletalAnimator::OnDestroy() {
	SceneManager::UnregisterSkeletalAnimator(this);
}


void ou::SkeletalAnimator::Serialize(Serializer &serializer) {
	Behavior::Serialize(serializer);

	auto clip = clip_;
	serializer.Asset("clip", clip);
	serializer.Field("playAutomatically", playAutomatically_);

	auto wrapMode = wrapMode_;
	serializer.Enum("wrapMode", wrapMode);

	auto speed = GetSpeed();
	serializer.Field("speed", speed);

	if (serializer.IsReading()) {
		if (clip != clip_) { SetClip(std::move(clip)); }
		SetWrapMode(wrapMode);
		SetSpeed(speed);
	}
}


void ou::SkeletalAnimator::Play(std::shared_ptr<SkeletalAnimationClip> clip) {
	if (!clip) {
		Log::Error("SkeletalAnimator::Play のクリップが null です");
		return;
	}

	RefreshBindings();
	RestoreRestPose();
	ClearFade();

	StartClip(std::move(clip));
}


void ou::SkeletalAnimator::CrossFade(std::shared_ptr<SkeletalAnimationClip> clip, float fadeLength) {
	if (!clip || fadeLength <= 0.0f) {
		Play(std::move(clip));
		return;
	}

	RefreshBindings();

	auto fadeOutBones = std::move(fadeOutBones_);
	for (auto &bone : fadeOutBones) {
		if (bone.transform) { bone.from = ReadTransform(*bone.transform); }
	}
	if (clip_) {
		const auto &tracks = clip_->GetTracks();
		for (size_t i = 0; i < tracks.size(); i++) {
			const auto &binding = bindings_[i];
			if (binding.transform) {
				fadeOutBones.push_back({tracks[i].path, binding.transform, ReadTransform(*binding.transform), binding.rest});
			}
		}
	}

	StartClip(std::move(clip));

	std::unordered_set<std::string> newPaths;
	for (const auto &track : clip_->GetTracks()) {
		newPaths.insert(track.path);
	}
	std::erase_if(fadeOutBones, [&](const FadeOutBone &bone) {
		return !bone.transform || newPaths.contains(bone.path);
	});
	fadeOutBones_ = std::move(fadeOutBones);

	fadeFromPoses_.clear();
	for (const auto &binding : bindings_) {
		fadeFromPoses_.push_back(binding.transform ? ReadTransform(*binding.transform) : BoneTransform{});
	}

	fadeLength_ = fadeLength;
	fadeElapsed_ = 0.0f;
}


void ou::SkeletalAnimator::Stop() {
	isPlaying_ = false;
	playhead_.Seek(0.0f);
	ClearFade();
}


bool ou::SkeletalAnimator::IsPlaying() const {
	return isPlaying_;
}


void ou::SkeletalAnimator::UpdateAnimation(float deltaTime) {
	if (!isPlaying_) { return; }

	RefreshBindings();

	const auto step = playhead_.Advance(deltaTime, clip_->GetLength());

	fadeElapsed_ += deltaTime;
	const auto fadeWeight = fadeLength_ > 0.0f ? Mathf::Clamp01(fadeElapsed_ / fadeLength_) : 1.0f;

	ApplyClip(playhead_.GetTime(), step.isFinished ? 1.0f : fadeWeight);

	if (fadeWeight >= 1.0f) { ClearFade(); }
	if (step.isFinished) { Stop(); }
}


void ou::SkeletalAnimator::SetClip(std::shared_ptr<SkeletalAnimationClip> clip) {
	Stop();
	clip_ = std::move(clip);
}


const std::shared_ptr<SkeletalAnimationClip> &ou::SkeletalAnimator::GetClip() const {
	return clip_;
}


void ou::SkeletalAnimator::SetTime(float time) {
	playhead_.Seek(time);
}


float ou::SkeletalAnimator::GetTime() const {
	return playhead_.GetTime();
}


void ou::SkeletalAnimator::SetSpeed(float speed) {
	playhead_.SetSpeed(speed);
}


float ou::SkeletalAnimator::GetSpeed() const {
	return playhead_.GetSpeed();
}


void ou::SkeletalAnimator::SetWrapMode(WrapMode wrapMode) {
	wrapMode_ = wrapMode;
	playhead_.SetWrapMode(wrapMode);
}


WrapMode ou::SkeletalAnimator::GetWrapMode() const {
	return playhead_.GetWrapMode();
}


void ou::SkeletalAnimator::SetPlayAutomatically(bool value) {
	playAutomatically_ = value;
}


bool ou::SkeletalAnimator::IsPlayAutomatically() const {
	return playAutomatically_;
}


void ou::SkeletalAnimator::StartClip(std::shared_ptr<SkeletalAnimationClip> clip) {
	clip_ = std::move(clip);
	BindClip(true);

	const auto clipWrapMode = clip_->GetWrapMode();
	playhead_.SetWrapMode(clipWrapMode != WrapMode::Default ? clipWrapMode : wrapMode_);
	playhead_.Restart();
	if (playhead_.GetSpeed() < 0.0f) { playhead_.Seek(clip_->GetLength()); }

	isPlaying_ = true;
}


void ou::SkeletalAnimator::BindClip(bool warnsMissingBone) {
	bindings_.clear();
	bindingGeneration_ = Scene::GetDestroyGeneration();
	if (!clip_) { return; }

	for (const auto &track : clip_->GetTracks()) {
		auto *transform = GetTransform()->Find(track.path);
		if (!transform) {
			if (warnsMissingBone) {
				Log::Warning("ボーン \"{}\" が見つかりません: {}", track.path, clip_->GetName());
			}
			bindings_.push_back({});
			continue;
		}


		const auto [rest, isInserted] = restPoses_.try_emplace(
			track.path, BoneTransform(transform->GetLocalPosition(), transform->GetLocalRotation(), transform->GetLocalScale()));
		bindings_.push_back({transform, rest->second});
	}
}


void ou::SkeletalAnimator::RefreshBindings() {
	if (bindingGeneration_ == Scene::GetDestroyGeneration()) { return; }

	BindClip(false);
	for (auto &bone : fadeOutBones_) {
		bone.transform = GetTransform()->Find(bone.path);
	}
}


void ou::SkeletalAnimator::RestoreRestPose() {
	for (const auto &binding : bindings_) {
		if (binding.transform) { WriteTransform(*binding.transform, binding.rest); }
	}
	for (const auto &bone : fadeOutBones_) {
		if (bone.transform) { WriteTransform(*bone.transform, bone.rest); }
	}
}


void ou::SkeletalAnimator::ClearFade() {
	fadeFromPoses_.clear();
	fadeOutBones_.clear();
	fadeLength_ = 0.0f;
	fadeElapsed_ = 0.0f;
}


void ou::SkeletalAnimator::ApplyClip(float time, float fadeWeight) {
	const auto &tracks = clip_->GetTracks();
	for (size_t i = 0; i < tracks.size(); i++) {
		const auto &binding = bindings_[i];
		if (!binding.transform) { continue; }

		auto value = tracks[i].Evaluate(time, binding.rest);
		if (fadeWeight < 1.0f) { value = BlendBone(fadeFromPoses_[i], value, fadeWeight); }
		WriteTransform(*binding.transform, value);
	}

	for (const auto &bone : fadeOutBones_) {
		if (bone.transform) { WriteTransform(*bone.transform, BlendBone(bone.from, bone.rest, fadeWeight)); }
	}
}
