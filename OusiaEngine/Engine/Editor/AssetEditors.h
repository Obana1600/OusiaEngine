#pragma once

#include "Audio/XAudio2/Audio.h"
#include "Editor/Editor.h"

#include <cstdint>
#include <string>
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


// Serialize を持つアセット（.mat / .physicsmaterial）の Editor の共通部分
// 欄で変えた値は読み込み済みのオブジェクトにすぐ入り（使っている Renderer などにもすぐ効く）、ファイルにも書き戻す
class SerializedAssetEditor: public Editor {
public: // メンバ関数

	~SerializedAssetEditor() override;


protected: // メンバ関数

	void OnEnable() override;

	// Assets の中のメインのアセットなら true（モデルの中のマテリアルや EngineAssets のものは書き換えない）
	bool IsEditable() const;

	// 書き換えられないときに、欄の上に理由を出す
	void DrawReadOnlyNote() const;

	// ApplyModifiedProperties の結果を渡す。ドラッグ中は毎フレーム書かないように、手を離してから書く
	void SaveWhenIdle(bool isModified);


private: // メンバ変数

	std::string assetPath_; // 書き換えられないなら空
	bool hasUnsavedChanges_ = false;


private: // メンバ関数

	void Save();
};


// .physicsmaterial
class PhysicsMaterialEditor final: public SerializedAssetEditor {
public: // メンバ関数

	void OnInspectorGui() override;
};


// .mat
class MaterialEditor final: public SerializedAssetEditor {
public: // メンバ関数

	void OnInspectorGui() override;
};


// .shader。Unity と同じく Inspector では編集しない（ファイルを書き換える）。中身を見せて、ファイルを開けるようにする
class ShaderEditor final: public Editor {
public: // メンバ関数

	void OnInspectorGui() override;
};

} // namespace ou
