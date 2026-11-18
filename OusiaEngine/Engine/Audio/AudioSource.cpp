#include "AudioSource.h"
#include "Audio/AudioClip.h"
#include "Diagnostics/Log.h"
#include "Math/Mathf.h"
#include "Serialize/Serializer.h"
#include "Render/Gizmos.h"
#include "Scene/Transform.h"

#include <utility>

using namespace ou;


void ou::AudioSource::Start() {
	if (playOnAwake_ && clip_) {
		Play();
	}
}


void ou::AudioSource::Update() {
	std::erase_if(oneShots_, [](const auto &oneShot) {
		return !Audio::IsPlaying(&oneShot.instance) &&
			   !Audio::IsPaused(&oneShot.instance);
	});
}


void ou::AudioSource::Serialize(Serializer &serializer) {
	// clip と submixType はインスタンスを作り直すので、今の値を写してから運び、AwakeFromLoad で比べる
	serializedClip_ = clip_;
	serializedSubmixType_ = submixType_;

	serializer.Asset("clip", serializedClip_);
	serializer.Field("volume", volume_);
	serializer.Field("pitch", pitch_);
	serializer.Field("loop", loop_);
	serializer.Field("mute", mute_);
	serializer.Field("playOnAwake", playOnAwake_);
	serializer.Enum("submixType", serializedSubmixType_);
}


void ou::AudioSource::AwakeFromLoad() {
	Component::AwakeFromLoad();

	volume_ = Mathf::Clamp01(volume_);

	if (serializedSubmixType_ == Audio::SubmixType::Count) {
		Log::Warning("submixType に Count は使えません");
		serializedSubmixType_ = submixType_;
	}

	if (serializedClip_ != clip_ || serializedSubmixType_ != submixType_) {
		clip_ = serializedClip_;
		submixType_ = serializedSubmixType_;
		RecreateInstance();
	}
	ApplySettingsToAll();
}


void ou::AudioSource::Play() {
	if (!instance_.IsValid()) { return; }

	// 一時停止中でも最初から鳴らす
	Audio::Stop(&instance_);
	instance_.loop = loop_;
	Audio::Play(&instance_);
}


void ou::AudioSource::Stop() {
	Audio::Stop(&instance_);
	oneShots_.clear();
}


void ou::AudioSource::Pause() {
	Audio::Pause(&instance_);
	for (auto &oneShot : oneShots_) {
		Audio::Pause(&oneShot.instance);
	}
}


void ou::AudioSource::UnPause() {
	if (Audio::IsPaused(&instance_)) {
		Audio::Play(&instance_);
	}
	for (auto &oneShot : oneShots_) {
		if (Audio::IsPaused(&oneShot.instance)) {
			Audio::Play(&oneShot.instance);
		}
	}
}


void ou::AudioSource::PlayOneShot(const std::shared_ptr<AudioClip> &clip, float volumeScale) {
	if (!clip) { return; }

	OneShot oneShot{};
	oneShot.volumeScale = volumeScale;
	oneShot.instance.type = submixType_;
	if (!Audio::CreateSoundInstance(&clip->GetSound(), &oneShot.instance)) { return; }

	ApplySettings(oneShot.instance, volumeScale);
	Audio::Play(&oneShot.instance);
	oneShots_.push_back(std::move(oneShot));
}


bool ou::AudioSource::IsPlaying() const {
	return Audio::IsPlaying(&instance_);
}


void ou::AudioSource::SetClip(std::shared_ptr<AudioClip> clip) {
	if (clip_ == clip) { return; }

	clip_ = std::move(clip);
	RecreateInstance();
}


const std::shared_ptr<AudioClip> &ou::AudioSource::GetClip() const {
	return clip_;
}


void ou::AudioSource::SetVolume(float volume) {
	volume_ = Mathf::Clamp01(volume);
	ApplySettingsToAll();
}


float ou::AudioSource::GetVolume() const {
	return volume_;
}


void ou::AudioSource::SetPitch(float pitch) {
	pitch_ = pitch;
	ApplySettingsToAll();
}


float ou::AudioSource::GetPitch() const {
	return pitch_;
}


void ou::AudioSource::SetLoop(bool loop) {
	loop_ = loop;
}


bool ou::AudioSource::GetLoop() const {
	return loop_;
}


void ou::AudioSource::SetMute(bool mute) {
	mute_ = mute;
	ApplySettingsToAll();
}


bool ou::AudioSource::GetMute() const {
	return mute_;
}


void ou::AudioSource::SetPlayOnAwake(bool playOnAwake) {
	playOnAwake_ = playOnAwake;
}


bool ou::AudioSource::GetPlayOnAwake() const {
	return playOnAwake_;
}


void ou::AudioSource::SetSubmixType(Audio::SubmixType submixType) {
	if (submixType_ == submixType) { return; }

	submixType_ = submixType;
	RecreateInstance();
}


Audio::SubmixType ou::AudioSource::GetSubmixType() const {
	return submixType_;
}


void ou::AudioSource::RecreateInstance() {
	instance_ = {};
	if (!clip_) { return; }

	instance_.type = submixType_;
	if (!Audio::CreateSoundInstance(&clip_->GetSound(), &instance_)) { return; }

	ApplySettings(instance_, 1.0f);
}


void ou::AudioSource::ApplySettings(Audio::SoundInstance &instance, float volumeScale) const {
	Audio::SetVolume(mute_ ? 0.0f : volume_ * volumeScale, &instance);
	Audio::SetPitch(pitch_, &instance);
}


void ou::AudioSource::ApplySettingsToAll() {
	ApplySettings(instance_, 1.0f);
	for (auto &oneShot : oneShots_) {
		ApplySettings(oneShot.instance, oneShot.volumeScale);
	}
}


void ou::AudioSource::OnDrawGizmos() {
	Gizmos::DrawIcon(GetTransform()->GetWorldPosition(), "AudioSource");
}
