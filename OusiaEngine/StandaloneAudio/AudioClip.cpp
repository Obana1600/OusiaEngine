#include "AudioClip.h"

#include <utility>


std::shared_ptr<AudioClip> AudioClip::Load(const std::filesystem::path &path) {
	Audio::Sound sound;
	if (!Audio::CreateSound(path, &sound)) { return nullptr; }

	return std::make_shared<AudioClip>(std::move(sound));
}


AudioClip::AudioClip(Audio::Sound sound)
	: sound_(std::move(sound)),
	  info_(Audio::GetSoundInfo(sound_)) {}


const Audio::Sound &AudioClip::GetSound() const {
	return sound_;
}


float AudioClip::GetLength() const {
	if (info_.sampleRate == 0) { return 0.0f; }
	return static_cast<float>(static_cast<double>(info_.sampleFrames) / info_.sampleRate);
}


uint32_t AudioClip::GetFrequency() const {
	return info_.sampleRate;
}


uint32_t AudioClip::GetChannels() const {
	return info_.channels;
}


uint64_t AudioClip::GetSamples() const {
	return info_.sampleFrames;
}
