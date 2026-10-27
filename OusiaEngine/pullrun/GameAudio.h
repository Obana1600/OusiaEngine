#pragma once

#include "Scene/Component.h"

#include <array>
#include <cstdint>
#include <memory>


namespace ou {

// 前方宣言
class AudioClip;
class AudioSource;
class Scene;

enum class Bgm : uint8_t {
	Title,
	BossNormal,
	BossHard,
	Clear,
	GameOver,
	Count,
};

enum class Se : uint8_t {
	BossBigBullet,
	BossCharge,
	BossFollowBullet,
	BossPowerUp,
	BossSmallBullet,
	PlayerBound,
	PlayerDash,
	PlayerDealDamage,
	PlayerDealDamageMaxCharge,
	PlayerTakeDamage,
	SystemClick,
	Count,
};

// BGM と SE を鳴らす。シーンをまたいで残る（ボタンの音がシーンの切り替えで途切れないように）
class GameAudio final: public Component {
public: // 静的メンバ関数

	// まだなければ作る。各シーンの組み立てで呼ぶ
	static void EnsureCreated(Scene &scene);

	// 同じ曲が流れていれば、最初からにはしない
	static void PlayBgm(Bgm bgm, bool loop = true);
	// volume は表の音量にさらに掛ける倍率（当たりの強さなどで変えたいとき）
	static void PlaySe(Se se, float volume = 1.0f);


public: // メンバ関数

	void Awake() override;
	void Update() override;
	void OnDestroy() override;


private: // 静的メンバ変数

	static inline GameAudio *sInstance_ = nullptr;


private: // メンバ変数

	AudioSource *bgmSource_ = nullptr;
	AudioSource *seSource_ = nullptr;
	std::array<std::shared_ptr<AudioClip>, static_cast<size_t>(Bgm::Count)> bgmClips_;
	std::array<std::shared_ptr<AudioClip>, static_cast<size_t>(Se::Count)> seClips_;
	// 同じ SE を続けて鳴らさないための残り時間（弾幕で同じ音が重なりすぎないように）
	std::array<float, static_cast<size_t>(Se::Count)> seCooldowns_{};
	Bgm currentBgm_ = Bgm::Count;
};

} // namespace ou
