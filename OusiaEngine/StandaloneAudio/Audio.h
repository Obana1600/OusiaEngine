#pragma once

// 単体で使えるオーディオ (XAudio2 + Media Foundation)
// 要件: Windows / C++17
// 使い方: Audio::Initialize() → AudioClip::Load() → AudioSource で再生 → Audio::Finalize()

#include <cstdint>
#include <filesystem>
#include <memory>


namespace Audio {

// 音量を変更する単位
enum class SubmixType {
	SoundEffect,
	Music,
	Count,
};

// デコード済みの音声データ
struct Sound {
	std::shared_ptr<void> internalState;

	bool IsValid() const {
		return internalState != nullptr;
	}
};

// Sound を鳴らす実体
struct SoundInstance {
	std::shared_ptr<void> internalState;
	SubmixType type = SubmixType::SoundEffect;
	bool loop = false;

	bool IsValid() const {
		return internalState != nullptr;
	}
};

// Sound の情報
struct SoundInfo {
	uint32_t sampleRate = 0;
	uint32_t channels = 0;
	uint64_t sampleFrames = 0;
};


// ----- 生成 -----

// 他のエンジンが XAudio2 / COM / Media Foundation を初期化済みでも呼んでよい
// COM のモードを先に決めてしまわないよう、他のエンジンの初期化より後に呼ぶ
bool Initialize();
void Finalize();

bool CreateSound(const std::filesystem::path &path, Sound *sound);
SoundInfo GetSoundInfo(const Sound &sound);

bool CreateSoundInstance(const Sound *sound, SoundInstance *instance);


// ----- 再生 -----

void Play(SoundInstance *instance);
void Pause(SoundInstance *instance);
void Stop(SoundInstance *instance);
bool IsPlaying(const SoundInstance *instance);
bool IsPaused(const SoundInstance *instance);

void SetVolume(float volume, SoundInstance *instance = nullptr);
float GetVolume(const SoundInstance *instance = nullptr);

void SetSubmixVolume(SubmixType type, float volume);
float GetSubmixVolume(SubmixType type);

void SetPitch(float pitch, SoundInstance *instance);
float GetPitch(const SoundInstance *instance);

} // namespace Audio
