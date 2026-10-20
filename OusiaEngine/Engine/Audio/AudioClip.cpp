#include "AudioClip.h"

#include <utility>

using namespace ou;


ou::AudioClip::AudioClip(Audio::Sound sound)
	: sound_(std::move(sound)),
	  info_(Audio::GetSoundInfo(sound_)) {}


const Audio::Sound &ou::AudioClip::GetSound() const {
	return sound_;
}


float ou::AudioClip::GetLength() const {
	if (info_.sampleRate == 0) { return 0.0f; }
	return static_cast<float>(static_cast<double>(info_.sampleFrames) / info_.sampleRate);
}


uint32_t ou::AudioClip::GetFrequency() const {
	return info_.sampleRate;
}


uint32_t ou::AudioClip::GetChannels() const {
	return info_.channels;
}


uint64_t ou::AudioClip::GetSamples() const {
	return info_.sampleFrames;
}
