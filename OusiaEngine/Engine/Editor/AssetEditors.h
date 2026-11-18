#pragma once

#include "Audio/XAudio2/Audio.h"
#include "Editor/Editor.h"

#include <cstdint>
#include <vector>


// アセット（GameObject 以外）の Inspector を描く Editor（Unity の TextureInspector・AudioClipInspector など）
// 登録は CustomEditor.cpp の RegisterEngineEditors
namespace ou {

// .png / .jpg。Preview に絵を出す
class TextureEditor final: public Editor {
public: // メンバ関数

	void OnInspectorGui() override;

	bool HasPreviewGui() const override;
	void OnPreviewGui(const Rect &rect) override;
};


// .wav / .mp3。Preview に波形を出し、鳴らせるようにする
class AudioClipEditor final: public Editor {
public: // メンバ関数

	~AudioClipEditor() override;

	void OnInspectorGui() override;

	bool HasPreviewGui() const override;
	void OnPreviewSettings() override;
	void OnPreviewGui(const Rect &rect) override;


private: // メンバ変数

	Audio::SoundInstance instance_;
	bool loops_ = false;

	// 波形は横幅が変わったときだけ作り直す（チャンネルごと）
	uint32_t peakCount_ = 0;
	std::vector<std::vector<Audio::WaveformPeak>> peaks_;


private: // メンバ関数

	void Play();
	void Stop();
	bool IsPlaying() const;
};

} // namespace ou
