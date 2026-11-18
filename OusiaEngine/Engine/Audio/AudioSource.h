#pragma once

#include "Audio/XAudio2/Audio.h"
#include "Scene/Component.h"

#include <memory>
#include <vector>


namespace ou {

// 前方宣言
class AudioClip;

class AudioSource: public Component {
public: // メンバ関数

	void Start() override;
	void Update() override;
	void Serialize(Serializer &serializer) override;

	void Play();
	void Stop();
	void Pause();
	void UnPause();

	void PlayOneShot(const std::shared_ptr<AudioClip> &clip, float volumeScale);

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

	void SetPlayOnAwake(bool playOnAwake);
	bool GetPlayOnAwake() const;

	void SetSubmixType(Audio::SubmixType submixType);
	Audio::SubmixType GetSubmixType() const;


protected: // メンバ関数

	void AwakeFromLoad() override;


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
	bool playOnAwake_ = true;
	Audio::SubmixType submixType_ = Audio::SubmixType::SoundEffect;

	// 読んだ clip と submixType（違えば AwakeFromLoad でインスタンスを作り直す）
	std::shared_ptr<AudioClip> serializedClip_;
	Audio::SubmixType serializedSubmixType_ = Audio::SubmixType::SoundEffect;


private: // メンバ関数

	void RecreateInstance();

	void ApplySettings(Audio::SoundInstance &instance, float volumeScale) const;
	void ApplySettingsToAll();
};

} // namespace ou
