#pragma once

#include <cstdint>
#include <filesystem>
#include <memory>


namespace ou::Audio {

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

void SetVolume(float volume, SoundInstance *instance = nullptr);
float GetVolume(const SoundInstance *instance = nullptr);

void SetSubmixVolume(SubmixType type, float volume);
float GetSubmixVolume(SubmixType type);

void SetPitch(float pitch, SoundInstance *instance);
float GetPitch(const SoundInstance *instance);

} // namespace ou::Audio
