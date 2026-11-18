#pragma once

#include "Animation/Core/AnimationPlayhead.h"
#include "Animation/Core/WrapMode.h"
#include "Animation/Skeletal/BoneAnimationTrack.h"
#include "Scene/Behavior.h"

#include <cstdint>
#include <memory>
#include <string>
#include <unordered_map>
#include <vector>


namespace ou {

// 前方宣言
class Transform;
class SkeletalAnimationClip;

class SkeletalAnimator final: public Behavior {
public: // メンバ関数

	void Awake() override;
	void Start() override;
	void OnDestroy() override;
	void Serialize(Serializer &serializer) override;

	void Play(std::shared_ptr<SkeletalAnimationClip> clip);
	void CrossFade(std::shared_ptr<SkeletalAnimationClip> clip, float fadeLength);
	void Stop();
	bool IsPlaying() const;

	void UpdateAnimation(float deltaTime);

	void SetClip(std::shared_ptr<SkeletalAnimationClip> clip);
	const std::shared_ptr<SkeletalAnimationClip> &GetClip() const;

	void SetTime(float time);
	float GetTime() const;

	void SetSpeed(float speed);
	float GetSpeed() const;

	void SetWrapMode(WrapMode wrapMode);
	WrapMode GetWrapMode() const;

	void SetPlayAutomatically(bool value);
	bool IsPlayAutomatically() const;


protected: // メンバ関数

	void AwakeFromLoad() override;


private: // インナークラス

	struct BoneBinding {
		Transform *transform = nullptr;
		BoneTransform rest;
	};

	struct FadeOutBone {
		std::string path;
		Transform *transform = nullptr;
		BoneTransform from;
		BoneTransform rest;
	};


private: // メンバ変数

	std::shared_ptr<SkeletalAnimationClip> clip_;
	std::vector<BoneBinding> bindings_;
	std::unordered_map<std::string, BoneTransform> restPoses_;
	uint64_t bindingGeneration_ = 0;

	AnimationPlayhead playhead_;
	WrapMode wrapMode_ = WrapMode::Default;
	bool playAutomatically_ = true;
	bool isPlaying_ = false;

	std::vector<BoneTransform> fadeFromPoses_;
	std::vector<FadeOutBone> fadeOutBones_;
	float fadeLength_ = 0.0f;
	float fadeElapsed_ = 0.0f;

	// 読んだ clip と speed（clip は違えば止めて付け替え、speed は再生位置の側に入れる）
	std::shared_ptr<SkeletalAnimationClip> serializedClip_;
	float serializedSpeed_ = 1.0f;


private: // メンバ関数

	void StartClip(std::shared_ptr<SkeletalAnimationClip> clip);
	void BindClip(bool warnsMissingBone);
	void RefreshBindings();
	void RestoreRestPose();
	void ClearFade();
	void ApplyClip(float time, float fadeWeight);
};

} // namespace ou
