#include "AudioSource.h"
#include "AudioClip.h"

#include <algorithm>
#include <utility>


void AudioSource::Play() {
	if (!instance_.IsValid()) { return; }

	// 一時停止中でも最初から鳴らす
	Audio::Stop(&instance_);
	instance_.loop = loop_;
	Audio::Play(&instance_);
}


void AudioSource::Stop() {
	Audio::Stop(&instance_);
	oneShots_.clear();
}


void AudioSource::Pause() {
	Audio::Pause(&instance_);
	for (auto &oneShot : oneShots_) {
		Audio::Pause(&oneShot.instance);
	}
}


void AudioSource::UnPause() {
	if (Audio::IsPaused(&instance_)) {
		Audio::Play(&instance_);
	}
	for (auto &oneShot : oneShots_) {
		if (Audio::IsPaused(&oneShot.instance)) {
			Audio::Play(&oneShot.instance);
		}
	}
}


void AudioSource::PlayOneShot(const std::shared_ptr<AudioClip> &clip, float volumeScale) {
	if (!clip) { return; }

	// Update が無いので鳴らすときに掃除する
	RemoveFinishedOneShots();

	OneShot oneShot{};
	oneShot.volumeScale = volumeScale;
	oneShot.instance.type = submixType_;
	if (!Audio::CreateSoundInstance(&clip->GetSound(), &oneShot.instance)) { return; }

	ApplySettings(oneShot.instance, volumeScale);
	Audio::Play(&oneShot.instance);
	oneShots_.push_back(std::move(oneShot));
}


bool AudioSource::IsPlaying() const {
	return Audio::IsPlaying(&instance_);
}


void AudioSource::SetClip(std::shared_ptr<AudioClip> clip) {
	if (clip_ == clip) { return; }

	clip_ = std::move(clip);
	RecreateInstance();
}


const std::shared_ptr<AudioClip> &AudioSource::GetClip() const {
	return clip_;
}


void AudioSource::SetVolume(float volume) {
	volume_ = std::clamp(volume, 0.0f, 1.0f);
	ApplySettingsToAll();
}


float AudioSource::GetVolume() const {
	return volume_;
}


void AudioSource::SetPitch(float pitch) {
	pitch_ = pitch;
	ApplySettingsToAll();
}


float AudioSource::GetPitch() const {
	return pitch_;
}


void AudioSource::SetLoop(bool loop) {
	loop_ = loop;
}


bool AudioSource::GetLoop() const {
	return loop_;
}


void AudioSource::SetMute(bool mute) {
	mute_ = mute;
	ApplySettingsToAll();
}


bool AudioSource::GetMute() const {
	return mute_;
}


void AudioSource::SetSubmixType(Audio::SubmixType submixType) {
	if (submixType_ == submixType) { return; }

	submixType_ = submixType;
	RecreateInstance();
}


Audio::SubmixType AudioSource::GetSubmixType() const {
	return submixType_;
}


void AudioSource::RecreateInstance() {
	instance_ = {};
	if (!clip_) { return; }

	instance_.type = submixType_;
	if (!Audio::CreateSoundInstance(&clip_->GetSound(), &instance_)) { return; }

	ApplySettings(instance_, 1.0f);
}


void AudioSource::RemoveFinishedOneShots() {
	const auto isFinished = [](const OneShot &oneShot) {
		return !Audio::IsPlaying(&oneShot.instance) &&
			   !Audio::IsPaused(&oneShot.instance);
	};
	oneShots_.erase(std::remove_if(oneShots_.begin(), oneShots_.end(), isFinished), oneShots_.end());
}


void AudioSource::ApplySettings(Audio::SoundInstance &instance, float volumeScale) const {
	Audio::SetVolume(mute_ ? 0.0f : volume_ * volumeScale, &instance);
	Audio::SetPitch(pitch_, &instance);
}


void AudioSource::ApplySettingsToAll() {
	ApplySettings(instance_, 1.0f);
	for (auto &oneShot : oneShots_) {
		ApplySettings(oneShot.instance, oneShot.volumeScale);
	}
}
