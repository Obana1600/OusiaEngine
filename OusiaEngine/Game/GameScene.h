#pragma once

#include "Audio/AudioClip.h"
#include "Audio/AudioSource.h"
#include "Particle/ParticleSystem.h"
#include "Particle/ParticleSystemRenderer.h"
#include "Physics/BoxCollider.h"
#include "Physics/SphereCollider.h"
#include "Render/Asset/Texture.h"
#include "Scene/Component.h"
#include "Scene/GameObject.h"

#include <memory>
#include <string>
#include <vector>


namespace ou {

void BuildGameScene(Scene &scene);

// 受け取ったトリガーのイベントを記録する（Physics の動作確認用）
class TriggerLogger final: public Component {
public: // メンバ関数

	void OnTriggerEnter(Collider *other) override;
	void OnTriggerStay(Collider *other) override;
	void OnTriggerExit(Collider *other) override;

	const std::vector<std::string> &GetEvents() const;
	void ClearEvents();

	// 最後に Stay を受け取ったフレーム。今フレームなら重なっている
	uint64_t GetLastStayFrame() const;


private: // メンバ変数

	std::vector<std::string> events_;
	uint64_t lastStayFrame_ = 0;


private: // メンバ関数

	void RecordEvent(const char *message, const Collider *other);
};

class GameSceneController final: public Component {
public: // メンバ関数

	void Awake() override;
	void Update() override;


private: // メンバ変数

	GameObject *emitter_ = nullptr;
	ParticleSystem *particleSystem_ = nullptr;
	ParticleSystemRenderer *particleRenderer_ = nullptr;

	// エミッタを円を描くように動かす（Local と World の違いを見るため）
	bool isEmitterMoving_ = false;

	// ImGui で選べるテクスチャ（nullptr は白）
	std::shared_ptr<Texture> uvCheckerTexture_;
	std::shared_ptr<Texture> softCircleTexture_;
	int textureIndex_ = 2;

	// 音
	AudioSource *bgmSource_ = nullptr;
	AudioSource *seSource_ = nullptr;
	std::shared_ptr<AudioClip> seClip_; // PlayOneShot で鳴らす

	// GameScene.h の private: に追加
	// DontDestroyOnLoad のシーンに残した BGM。読み込み直したときは、作らずにこれを使う
	static inline AudioSource *sBgmSource_ = nullptr;

	// トリガーの確認用。boxA だけがトリガーで、boxB と sphere が触れると Enter / Stay / Exit が来る
	struct PhysicsTarget {
		GameObject *gameObject = nullptr;
		Collider *collider = nullptr;
		TriggerLogger *logger = nullptr;
	};
	PhysicsTarget physicsTargets_[3];

private: // メンバ関数

	void CreatePhysicsTargets();

	// コライダーの位置や設定を ImGui で操作し、届いたイベントを表示する
	void ShowPhysicsWindow();


	void UpdateEmitterMotion();

	// パーティクルの設定を ImGui で操作する
	void ShowParticleWindow();

	// 音の再生と設定を ImGui で操作する
	void ShowAudioWindow();
};

} // namespace ou
