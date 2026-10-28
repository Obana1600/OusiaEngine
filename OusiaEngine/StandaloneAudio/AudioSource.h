#pragma once

#include "Audio.h"

#include <memory>
#include <vector>


// 前方宣言
class AudioClip;

class AudioSource {
public: // メンバ関数

	void Play();
	void Stop();
	void Pause();
	void UnPause();

	void PlayOneShot(const std::shared_ptr<AudioClip> &clip, float volumeScale = 1.0f);

	bool IsPlaying() const;

	void SetClip(std::shared_ptr<AudioClip> clip);
	const std::shared_ptr<AudioClip> &GetClip() const;

	void SetVolume(float volume);
	float GetVolume() const;

	void SetPitch(float pitch);
	float GetPitch() const;

	void SetLoop(bool loop);
	bool GetLoop() const;

	void SetMute(bool mute);
	bool GetMute() const;

	void SetSubmixType(Audio::SubmixType submixType);
	Audio::SubmixType GetSubmixType() const;


private: // インナークラス

	struct OneShot {
		Audio::SoundInstance instance;
		float volumeScale = 1.0f;
	};


private: // メンバ変数

	std::shared_ptr<AudioClip> clip_;
	Audio::SoundInstance instance_;
	std::vector<OneShot> oneShots_;

	float volume_ = 1.0f;
	float pitch_ = 1.0f;
	bool loop_ = false;
	bool mute_ = false;
	Audio::SubmixType submixType_ = Audio::SubmixType::SoundEffect;


private: // メンバ関数

	void RecreateInstance();
	void RemoveFinishedOneShots();

	void ApplySettings(Audio::SoundInstance &instance, float volumeScale) const;
	void ApplySettingsToAll();
};
