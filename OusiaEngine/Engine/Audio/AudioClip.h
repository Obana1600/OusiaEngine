#pragma once

#include "Audio/XAudio2/Audio.h"
#include "Core/Object.h"

#include <cstdint>


namespace ou {

class AudioClip: public Object {
public: // メンバ関数

	explicit AudioClip(Audio::Sound sound);

	const Audio::Sound &GetSound() const;

	float GetLength() const;	   // 秒
	uint32_t GetFrequency() const; // サンプリングレート (Hz)
	uint32_t GetChannels() const;
	uint64_t GetSamples() const; // 1 チャンネル当たりのサンプル数


private: // メンバ変数

	Audio::Sound sound_;
	Audio::SoundInfo info_;
};

} // namespace ou
