#pragma once

#include <cstdint>
#include <filesystem>
#include <memory>
#include <vector>


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


// 波形の 1 区間の振れ幅（-1～1）
struct WaveformPeak {
	float min = 0.0f;
	float max = 0.0f;
};


// ----- 生成 -----

bool Initialize();
void Finalize();

bool CreateSound(const std::filesystem::path &path, Sound *sound);
SoundInfo GetSoundInfo(const Sound &sound);

// 波形の表示用。channel の音を頭から bucketCount 個の区間に分け、区間ごとの最小・最大を返す
std::vector<WaveformPeak> GetWaveformPeaks(const Sound &sound, uint32_t channel, uint32_t bucketCount);

bool CreateSoundInstance(const Sound *sound, SoundInstance *instance);


// ----- 再生 -----

void Play(SoundInstance *instance);
void Pause(SoundInstance *instance);
void Stop(SoundInstance *instance);
bool IsPlaying(const SoundInstance *instance);
bool IsPaused(const SoundInstance *instance);

// 今どこを鳴らしているか（頭からのサンプルフレーム数。止まっていれば 0）
uint64_t GetPlaybackPosition(const SoundInstance *instance);

void SetVolume(float volume, SoundInstance *instance = nullptr);
float GetVolume(const SoundInstance *instance = nullptr);

void SetSubmixVolume(SubmixType type, float volume);
float GetSubmixVolume(SubmixType type);

void SetPitch(float pitch, SoundInstance *instance);
float GetPitch(const SoundInstance *instance);

} // namespace ou::Audio
