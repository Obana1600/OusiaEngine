#include "Audio.h"
#include "Core/StringUtil.h"
#include "Diagnostics/Fatal.h"
#include "Diagnostics/Log.h"
#include "Math/Mathf.h"

#include <algorithm>
#include <array>
#include <vector>

#include <mfapi.h>
#include <mfidl.h>
#include <mfreadwrite.h>
#include <wrl/client.h>
#include <xaudio2.h>

#pragma comment(lib, "xaudio2.lib")
#pragma comment(lib, "mfplat.lib")
#pragma comment(lib, "mfreadwrite.lib")
#pragma comment(lib, "mfuuid.lib")

using namespace ou;
using namespace Microsoft::WRL;


namespace {

// ピッチの上限 / 下限
constexpr float kMaxPitch = 4.0f;
constexpr float kMinPitch = XAUDIO2_MIN_FREQ_RATIO;

// グローバルなインスタンス
ComPtr<IXAudio2> sXAudio;
IXAudio2MasteringVoice *sMasteringVoice = nullptr;
std::array<IXAudio2SubmixVoice *, static_cast<size_t>(Audio::SubmixType::Count)> sSubmixVoices{};
bool sIsMediaFoundationStarted = false;

struct SoundInternal {
	std::vector<uint8_t> format;
	std::vector<uint8_t> audioData;

	const WAVEFORMATEX *GetFormat() const {
		return reinterpret_cast<const WAVEFORMATEX *>(format.data());
	}
};

enum class PlayState {
	Stopped,
	Playing,
	Paused,
};

struct SoundInstanceInternal {
	std::shared_ptr<SoundInternal> sound;
	IXAudio2SourceVoice *voice = nullptr;
	PlayState state = PlayState::Stopped;
	uint64_t startSamplesPlayed = 0; // 頭から鳴らしはじめたときの SamplesPlayed（再生位置はここからの差）

	~SoundInstanceInternal() {
		if (voice && sXAudio) {
			voice->DestroyVoice();
		}
	}
};

SoundInstanceInternal *ToInternal(const Audio::SoundInstance *instance) {
	if (!sXAudio || !instance || !instance->IsValid()) { return nullptr; }
	return static_cast<SoundInstanceInternal *>(instance->internalState.get());
}

} // namespace



bool ou::Audio::Initialize() {
	auto hr = MFStartup(MF_VERSION, MFSTARTUP_NOSOCKET);
	if (FAILED(hr)) {
		Log::Error("Media Foundation の初期化に失敗しました: {}", DescribeHResult(hr));
		return false;
	}
	sIsMediaFoundationStarted = true;

	hr = XAudio2Create(&sXAudio, 0, XAUDIO2_DEFAULT_PROCESSOR);
	if (FAILED(hr)) {
		Log::Error("XAudio2 の作成に失敗しました: {}", DescribeHResult(hr));
		Finalize();
		return false;
	}

	hr = sXAudio->CreateMasteringVoice(&sMasteringVoice);
	if (FAILED(hr)) {
		Log::Error("マスタリングボイスの作成に失敗しました: {}", DescribeHResult(hr));
		Finalize();
		return false;
	}

	XAUDIO2_VOICE_DETAILS details{};
	sMasteringVoice->GetVoiceDetails(&details);
	for (auto &submix : sSubmixVoices) {
		hr = sXAudio->CreateSubmixVoice(&submix, details.InputChannels, details.InputSampleRate);
		if (FAILED(hr)) {
			Log::Error("サブミックスボイスの作成に失敗しました: {}", DescribeHResult(hr));
			Finalize();
			return false;
		}
	}

	return true;
}


void ou::Audio::Finalize() {
	for (auto &submix : sSubmixVoices) {
		if (submix) {
			submix->DestroyVoice();
			submix = nullptr;
		}
	}

	if (sMasteringVoice) {
		sMasteringVoice->DestroyVoice();
		sMasteringVoice = nullptr;
	}

	sXAudio.Reset();

	if (sIsMediaFoundationStarted) {
		MFShutdown();
		sIsMediaFoundationStarted = false;
	}
}


bool ou::Audio::CreateSound(const std::filesystem::path &path, Sound *sound) {
	if (!sIsMediaFoundationStarted || !sound) { return false; }

	const auto pathUtf8 = ToUtf8(path);

	// ファイルを開く
	ComPtr<IMFSourceReader> reader;
	auto hr = MFCreateSourceReaderFromURL(path.c_str(), nullptr, &reader);
	if (FAILED(hr)) {
		Log::Error("音声ファイルを開けませんでした: {} {}", pathUtf8, DescribeHResult(hr));
		return false;
	}

	// 最初の音声ストリームだけを読む
	const auto stream = static_cast<DWORD>(MF_SOURCE_READER_FIRST_AUDIO_STREAM);
	reader->SetStreamSelection(static_cast<DWORD>(MF_SOURCE_READER_ALL_STREAMS), FALSE);
	reader->SetStreamSelection(stream, TRUE);

	// PCM 形式にフォーマット指定する
	ComPtr<IMFMediaType> pcmType;
	MFCreateMediaType(&pcmType);
	pcmType->SetGUID(MF_MT_MAJOR_TYPE, MFMediaType_Audio);
	pcmType->SetGUID(MF_MT_SUBTYPE, MFAudioFormat_PCM);
	hr = reader->SetCurrentMediaType(stream, nullptr, pcmType.Get());
	if (FAILED(hr)) {
		Log::Error("PCM にデコードできない形式です: {}: {}", pathUtf8, DescribeHResult(hr));
		return false;
	}

	// 実際にセットされたメディアタイプを取得する
	ComPtr<IMFMediaType> outputType;
	reader->GetCurrentMediaType(stream, &outputType);

	// WAVEFORMATEX を取得する
	WAVEFORMATEX *waveFormat = nullptr;
	UINT32 formatSize = 0;
	hr = MFCreateWaveFormatExFromMFMediaType(outputType.Get(), &waveFormat, &formatSize);
	if (FAILED(hr)) {
		Log::Error("音声の形式を取得できませんでした: {}: {}", pathUtf8, DescribeHResult(hr));
		return false;
	}

	// データを格納する準備
	auto internal = std::make_shared<SoundInternal>();
	const auto *formatBytes = reinterpret_cast<const uint8_t *>(waveFormat);
	internal->format.assign(formatBytes, formatBytes + formatSize);
	CoTaskMemFree(waveFormat);

	// PCM データのバッファを構築
	while (true) {
		DWORD flags = 0;
		ComPtr<IMFSample> sample;
		hr = reader->ReadSample(stream, 0, nullptr, &flags, nullptr, &sample);
		if (FAILED(hr)) {
			Log::Error("音声のデコードに失敗しました: {}: {}", pathUtf8, DescribeHResult(hr));
			return false;
		}
		if (flags & MF_SOURCE_READERF_ENDOFSTREAM) { break; }
		if (!sample) { continue; }

		ComPtr<IMFMediaBuffer> buffer;
		hr = sample->ConvertToContiguousBuffer(&buffer);
		if (FAILED(hr)) {
			Log::Error("音声のバッファを取得できませんでした: {}: {}", pathUtf8, DescribeHResult(hr));
			return false;
		}

		BYTE *data = nullptr;
		DWORD length = 0;
		buffer->Lock(&data, nullptr, &length);
		internal->audioData.insert(internal->audioData.end(), data, data + length);
		buffer->Unlock();
	}

	// 空のバッファは XAudio2 に渡せない
	if (internal->audioData.empty()) {
		Log::Error("音声のデータが空です: {}", pathUtf8);
		return false;
	}

	sound->internalState = std::move(internal);
	return true;
}


Audio::SoundInfo ou::Audio::GetSoundInfo(const Sound &sound) {
	const auto *internal = static_cast<const SoundInternal *>(sound.internalState.get());
	if (!internal) { return {}; }

	const auto *format = internal->GetFormat();
	SoundInfo info{};
	info.sampleRate = format->nSamplesPerSec;
	info.channels = format->nChannels;
	info.sampleFrames = internal->audioData.size() / format->nBlockAlign;
	return info;
}


std::vector<Audio::WaveformPeak> ou::Audio::GetWaveformPeaks(const Sound &sound, uint32_t channel, uint32_t bucketCount) {
	const auto *internal = static_cast<const SoundInternal *>(sound.internalState.get());
	if (!internal || bucketCount == 0) { return {}; }

	// Media Foundation に PCM（整数）で出させているので、整数の PCM だけを読む
	const auto *format = internal->GetFormat();
	const auto blockAlign = static_cast<size_t>(format->nBlockAlign);
	const auto bytesPerSample = static_cast<size_t>(format->wBitsPerSample / 8);
	if (blockAlign == 0 || bytesPerSample == 0 || bytesPerSample > 4 || channel >= format->nChannels) { return {}; }

	const auto *data = internal->audioData.data();
	const auto frames = internal->audioData.size() / blockAlign;
	const auto readSample = [&](size_t frame) {
		const auto *bytes = data + frame * blockAlign + channel * bytesPerSample;
		switch (bytesPerSample) {
		case 1: return (static_cast<float>(bytes[0]) - 128.0f) / 128.0f; // 8 ビットだけ符号なし
		case 2: return static_cast<float>(static_cast<int16_t>(bytes[0] | (bytes[1] << 8))) / 32768.0f;
		case 3: {
			const auto value = static_cast<int32_t>((static_cast<uint32_t>(bytes[0]) << 8) | (static_cast<uint32_t>(bytes[1]) << 16) | (static_cast<uint32_t>(bytes[2]) << 24));
			return static_cast<float>(value >> 8) / 8388608.0f;
		}
		default: {
			const auto value = static_cast<int32_t>(static_cast<uint32_t>(bytes[0]) | (static_cast<uint32_t>(bytes[1]) << 8) | (static_cast<uint32_t>(bytes[2]) << 16) | (static_cast<uint32_t>(bytes[3]) << 24));
			return static_cast<float>(static_cast<double>(value) / 2147483648.0);
		}
		}
	};

	std::vector<WaveformPeak> peaks(bucketCount);
	for (uint32_t i = 0; i < bucketCount; i++) {
		const auto begin = frames * i / bucketCount;
		const auto end = std::max(frames * (i + 1) / bucketCount, begin + 1);
		if (begin >= frames) { break; }

		auto &peak = peaks[i];
		peak.min = peak.max = readSample(begin);
		for (auto frame = begin + 1; frame < std::min(end, frames); frame++) {
			const auto sample = readSample(frame);
			peak.min = std::min(peak.min, sample);
			peak.max = std::max(peak.max, sample);
		}
	}
	return peaks;
}


bool ou::Audio::CreateSoundInstance(const Sound *sound, SoundInstance *instance) {
	if (!sXAudio || !sound || !sound->IsValid() || !instance) { return false; }

	auto internal = std::make_shared<SoundInstanceInternal>();
	internal->sound = std::static_pointer_cast<SoundInternal>(sound->internalState);

	XAUDIO2_SEND_DESCRIPTOR send{0, sSubmixVoices[static_cast<size_t>(instance->type)]};
	XAUDIO2_VOICE_SENDS sends{1, &send};

	auto hr = sXAudio->CreateSourceVoice(
		&internal->voice, internal->sound->GetFormat(), 0, kMaxPitch, nullptr, &sends);
	if (FAILED(hr)) {
		Log::Error("ソースボイスの作成に失敗しました: {}", DescribeHResult(hr));
		return false;
	}

	instance->internalState = std::move(internal);
	return true;
}


void ou::Audio::Play(SoundInstance *instance) {
	auto *internal = ToInternal(instance);
	if (!internal) { return; }

	// 一時停止中なら続きから
	if (internal->state == PlayState::Paused) {
		internal->voice->Start();
		internal->state = PlayState::Playing;
		return;
	}

	// 最初から鳴らしなおす
	internal->voice->Stop();
	internal->voice->FlushSourceBuffers();

	const auto &audioData = internal->sound->audioData;
	XAUDIO2_BUFFER buffer{};
	buffer.Flags = XAUDIO2_END_OF_STREAM;
	buffer.AudioBytes = static_cast<UINT>(audioData.size());
	buffer.pAudioData = audioData.data();
	buffer.LoopCount = instance->loop ? XAUDIO2_LOOP_INFINITE : 0;
	internal->voice->SubmitSourceBuffer(&buffer);

	// SamplesPlayed は途中で止めても 0 に戻らないので、鳴らしはじめの値を覚えておく
	XAUDIO2_VOICE_STATE state{};
	internal->voice->GetState(&state);
	internal->startSamplesPlayed = state.SamplesPlayed;

	internal->voice->Start();
	internal->state = PlayState::Playing;
}


void ou::Audio::Pause(SoundInstance *instance) {
	if (!IsPlaying(instance)) { return; }

	auto *internal = ToInternal(instance);
	internal->voice->Stop();
	internal->state = PlayState::Paused;
}


void ou::Audio::Stop(SoundInstance *instance) {
	auto *internal = ToInternal(instance);
	if (!internal) { return; }

	internal->voice->Stop();
	internal->voice->FlushSourceBuffers();
	internal->state = PlayState::Stopped;
}


bool ou::Audio::IsPlaying(const SoundInstance *instance) {
	const auto *internal = ToInternal(instance);
	if (!internal || internal->state != PlayState::Playing) { return false; }

	XAUDIO2_VOICE_STATE state{};
	internal->voice->GetState(&state, XAUDIO2_VOICE_NOSAMPLESPLAYED);
	return state.BuffersQueued > 0;
}


bool ou::Audio::IsPaused(const SoundInstance *instance) {
	const auto *internal = ToInternal(instance);
	return internal && internal->state == PlayState::Paused;
}


uint64_t ou::Audio::GetPlaybackPosition(const SoundInstance *instance) {
	const auto *internal = ToInternal(instance);
	if (!internal || internal->state == PlayState::Stopped) { return 0; }

	const auto blockAlign = internal->sound->GetFormat()->nBlockAlign;
	const auto frames = blockAlign ? internal->sound->audioData.size() / blockAlign : 0;
	if (frames == 0) { return 0; }

	// 終わりまで鳴ると SamplesPlayed は 0 に戻るので、そのときは終わりにしておく
	XAUDIO2_VOICE_STATE state{};
	internal->voice->GetState(&state);
	if (state.SamplesPlayed < internal->startSamplesPlayed) { return frames; }

	const auto played = state.SamplesPlayed - internal->startSamplesPlayed;
	return instance->loop ? played % frames : std::min<uint64_t>(played, frames);
}


void ou::Audio::SetVolume(float volume, SoundInstance *instance) {
	if (!sXAudio) { return; }

	if (!instance) {
		sMasteringVoice->SetVolume(volume);
	} else if (auto *internal = ToInternal(instance)) {
		internal->voice->SetVolume(volume);
	}
}


float ou::Audio::GetVolume(const SoundInstance *instance) {
	if (!sXAudio) { return 0.0f; }

	auto volume = 0.0f;
	if (!instance) {
		sMasteringVoice->GetVolume(&volume);
	} else if (const auto *internal = ToInternal(instance)) {
		internal->voice->GetVolume(&volume);
	}

	return volume;
}


void ou::Audio::SetEnginePaused(bool isPaused) {
	if (!sXAudio) { return; }
	if (isPaused) {
		sXAudio->StopEngine();
	} else if (FAILED(sXAudio->StartEngine())) {
		Log::Error("音の再開に失敗しました");
	}
}


void ou::Audio::SetSubmixVolume(SubmixType type, float volume) {
	if (!sXAudio) { return; }
	sSubmixVoices[static_cast<size_t>(type)]->SetVolume(volume);
}


float ou::Audio::GetSubmixVolume(SubmixType type) {
	if (!sXAudio) { return 0.0f; }

	auto volume = 0.0f;
	sSubmixVoices[static_cast<size_t>(type)]->GetVolume(&volume);
	return volume;
}


void ou::Audio::SetPitch(float pitch, SoundInstance *instance) {
	if (auto *internal = ToInternal(instance)) {
		internal->voice->SetFrequencyRatio(Mathf::Clamp(pitch, kMinPitch, kMaxPitch));
	}
}


float ou::Audio::GetPitch(const SoundInstance *instance) {
	auto pitch = 1.0f;
	if (const auto *internal = ToInternal(instance)) {
		internal->voice->GetFrequencyRatio(&pitch);
	}

	return pitch;
}
