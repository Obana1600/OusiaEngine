#include "Audio.h"

#include <algorithm>
#include <array>
#include <cwchar>
#include <string>
#include <vector>

#ifndef NOMINMAX
#define NOMINMAX
#endif
#include <Windows.h>
#include <mfapi.h>
#include <mfidl.h>
#include <mfreadwrite.h>
#include <wrl/client.h>
#include <xaudio2.h>

#pragma comment(lib, "xaudio2.lib")
#pragma comment(lib, "mfplat.lib")
#pragma comment(lib, "mfreadwrite.lib")
#pragma comment(lib, "mfuuid.lib")

using namespace Microsoft::WRL;


namespace {

// ピッチの上限 / 下限
constexpr float kMaxPitch = 4.0f;
constexpr float kMinPitch = XAUDIO2_MIN_FREQ_RATIO;

// グローバルなインスタンス
ComPtr<IXAudio2> sXAudio;
IXAudio2MasteringVoice *sMasteringVoice = nullptr;
std::array<IXAudio2SubmixVoice *, static_cast<size_t>(Audio::SubmixType::Count)> sSubmixVoices{};
bool sIsComInitialized = false;
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

// エンジンのログの代わりに出力ウィンドウへ出す
void LogError(const std::wstring &message) {
	OutputDebugStringW((L"[Audio] " + message + L"\n").c_str());
}

std::wstring DescribeHResult(HRESULT hr) {
	wchar_t code[16]{};
	swprintf_s(code, L"0x%08x", static_cast<unsigned int>(hr));

	// HRESULTからエラーメッセージを取りだす
	wchar_t *buffer = nullptr;
	const auto length = FormatMessageW(
		FORMAT_MESSAGE_ALLOCATE_BUFFER | FORMAT_MESSAGE_FROM_SYSTEM | FORMAT_MESSAGE_IGNORE_INSERTS,
		nullptr, static_cast<DWORD>(hr), 0, reinterpret_cast<LPWSTR>(&buffer), 0, nullptr);

	auto text = (length > 0) ? std::wstring(buffer, length) : std::wstring(L"(説明なし)");
	LocalFree(buffer);

	// 末尾の改行を取り除く
	while (!text.empty() && (text.back() == L'\n' || text.back() == L'\r')) {
		text.pop_back();
	}

	return std::wstring(code) + L" " + text;
}

} // namespace



bool Audio::Initialize() {
	// 他で初期化済みなら S_FALSE / RPC_E_CHANGED_MODE が返るが、どちらでも COM は使える
	auto hr = CoInitializeEx(nullptr, COINIT_MULTITHREADED);
	sIsComInitialized = SUCCEEDED(hr);

	hr = MFStartup(MF_VERSION, MFSTARTUP_NOSOCKET);
	if (FAILED(hr)) {
		LogError(L"Media Foundation の初期化に失敗しました: " + DescribeHResult(hr));
		Finalize();
		return false;
	}
	sIsMediaFoundationStarted = true;

	// 他のエンジンとは別の XAudio2 を作り、出力は OS 側でミックスされる
	hr = XAudio2Create(&sXAudio, 0, XAUDIO2_DEFAULT_PROCESSOR);
	if (FAILED(hr)) {
		LogError(L"XAudio2 の作成に失敗しました: " + DescribeHResult(hr));
		Finalize();
		return false;
	}

	hr = sXAudio->CreateMasteringVoice(&sMasteringVoice);
	if (FAILED(hr)) {
		LogError(L"マスタリングボイスの作成に失敗しました: " + DescribeHResult(hr));
		Finalize();
		return false;
	}

	XAUDIO2_VOICE_DETAILS details{};
	sMasteringVoice->GetVoiceDetails(&details);
	for (auto &submix : sSubmixVoices) {
		hr = sXAudio->CreateSubmixVoice(&submix, details.InputChannels, details.InputSampleRate);
		if (FAILED(hr)) {
			LogError(L"サブミックスボイスの作成に失敗しました: " + DescribeHResult(hr));
			Finalize();
			return false;
		}
	}

	return true;
}


void Audio::Finalize() {
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

	// 自分で初期化した分だけ戻す
	if (sIsComInitialized) {
		CoUninitialize();
		sIsComInitialized = false;
	}
}


bool Audio::CreateSound(const std::filesystem::path &path, Sound *sound) {
	if (!sIsMediaFoundationStarted || !sound) { return false; }

	const auto &pathText = path.native();

	// ファイルを開く
	ComPtr<IMFSourceReader> reader;
	auto hr = MFCreateSourceReaderFromURL(path.c_str(), nullptr, &reader);
	if (FAILED(hr)) {
		LogError(L"音声ファイルを開けませんでした: " + pathText + L": " + DescribeHResult(hr));
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
		LogError(L"PCM にデコードできない形式です: " + pathText + L": " + DescribeHResult(hr));
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
		LogError(L"音声の形式を取得できませんでした: " + pathText + L": " + DescribeHResult(hr));
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
			LogError(L"音声のデコードに失敗しました: " + pathText + L": " + DescribeHResult(hr));
			return false;
		}
		if (flags & MF_SOURCE_READERF_ENDOFSTREAM) { break; }
		if (!sample) { continue; }

		ComPtr<IMFMediaBuffer> buffer;
		hr = sample->ConvertToContiguousBuffer(&buffer);
		if (FAILED(hr)) {
			LogError(L"音声のバッファを取得できませんでした: " + pathText + L": " + DescribeHResult(hr));
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
		LogError(L"音声のデータが空です: " + pathText);
		return false;
	}

	sound->internalState = std::move(internal);
	return true;
}


Audio::SoundInfo Audio::GetSoundInfo(const Sound &sound) {
	const auto *internal = static_cast<const SoundInternal *>(sound.internalState.get());
	if (!internal) { return {}; }

	const auto *format = internal->GetFormat();
	SoundInfo info{};
	info.sampleRate = format->nSamplesPerSec;
	info.channels = format->nChannels;
	info.sampleFrames = internal->audioData.size() / format->nBlockAlign;
	return info;
}


bool Audio::CreateSoundInstance(const Sound *sound, SoundInstance *instance) {
	if (!sXAudio || !sound || !sound->IsValid() || !instance) { return false; }

	auto internal = std::make_shared<SoundInstanceInternal>();
	internal->sound = std::static_pointer_cast<SoundInternal>(sound->internalState);

	XAUDIO2_SEND_DESCRIPTOR send{0, sSubmixVoices[static_cast<size_t>(instance->type)]};
	XAUDIO2_VOICE_SENDS sends{1, &send};

	auto hr = sXAudio->CreateSourceVoice(
		&internal->voice, internal->sound->GetFormat(), 0, kMaxPitch, nullptr, &sends);
	if (FAILED(hr)) {
		LogError(L"ソースボイスの作成に失敗しました: " + DescribeHResult(hr));
		return false;
	}

	instance->internalState = std::move(internal);
	return true;
}


void Audio::Play(SoundInstance *instance) {
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

	internal->voice->Start();
	internal->state = PlayState::Playing;
}


void Audio::Pause(SoundInstance *instance) {
	if (!IsPlaying(instance)) { return; }

	auto *internal = ToInternal(instance);
	internal->voice->Stop();
	internal->state = PlayState::Paused;
}


void Audio::Stop(SoundInstance *instance) {
	auto *internal = ToInternal(instance);
	if (!internal) { return; }

	internal->voice->Stop();
	internal->voice->FlushSourceBuffers();
	internal->state = PlayState::Stopped;
}


bool Audio::IsPlaying(const SoundInstance *instance) {
	const auto *internal = ToInternal(instance);
	if (!internal || internal->state != PlayState::Playing) { return false; }

	XAUDIO2_VOICE_STATE state{};
	internal->voice->GetState(&state, XAUDIO2_VOICE_NOSAMPLESPLAYED);
	return state.BuffersQueued > 0;
}


bool Audio::IsPaused(const SoundInstance *instance) {
	const auto *internal = ToInternal(instance);
	return internal && internal->state == PlayState::Paused;
}


void Audio::SetVolume(float volume, SoundInstance *instance) {
	if (!sXAudio) { return; }

	if (!instance) {
		sMasteringVoice->SetVolume(volume);
	} else if (auto *internal = ToInternal(instance)) {
		internal->voice->SetVolume(volume);
	}
}


float Audio::GetVolume(const SoundInstance *instance) {
	if (!sXAudio) { return 0.0f; }

	auto volume = 0.0f;
	if (!instance) {
		sMasteringVoice->GetVolume(&volume);
	} else if (const auto *internal = ToInternal(instance)) {
		internal->voice->GetVolume(&volume);
	}

	return volume;
}


void Audio::SetSubmixVolume(SubmixType type, float volume) {
	if (!sXAudio) { return; }
	sSubmixVoices[static_cast<size_t>(type)]->SetVolume(volume);
}


float Audio::GetSubmixVolume(SubmixType type) {
	if (!sXAudio) { return 0.0f; }

	auto volume = 0.0f;
	sSubmixVoices[static_cast<size_t>(type)]->GetVolume(&volume);
	return volume;
}


void Audio::SetPitch(float pitch, SoundInstance *instance) {
	if (auto *internal = ToInternal(instance)) {
		internal->voice->SetFrequencyRatio(std::clamp(pitch, kMinPitch, kMaxPitch));
	}
}


float Audio::GetPitch(const SoundInstance *instance) {
	auto pitch = 1.0f;
	if (const auto *internal = ToInternal(instance)) {
		internal->voice->GetFrequencyRatio(&pitch);
	}

	return pitch;
}
