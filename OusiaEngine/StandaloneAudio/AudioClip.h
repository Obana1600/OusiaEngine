#pragma once

#include "Audio.h"

#include <cstdint>
#include <filesystem>
#include <memory>


class AudioClip {
public: // 静的メンバ関数

	// 音声ファイルを読み込む (失敗したら nullptr)
	static std::shared_ptr<AudioClip> Load(const std::filesystem::path &path);


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
