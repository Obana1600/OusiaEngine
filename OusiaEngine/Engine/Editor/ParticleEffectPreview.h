#pragma once

#include "Math/Rect.h"

#include <cstdint>
#include <vector>


namespace ou {

// 前方宣言
class GameObject;
class ParticleSystem;

// Play していないときに、選んでいるパーティクルを Scene ビューで動かす（Unity の Scene ビューの「Particle Effect」）
// 選んだ GameObject から親へ ParticleSystem が続くかぎりさかのぼり、その下の ParticleSystem をまとめて動かす
// 粒はシーンに保存されないので、止めたり選び直したりすれば元に戻り、シーンも変更扱いにならない
class ParticleEffectPreview final {
public: // 静的メンバ関数

	ParticleEffectPreview() = delete;

	// 毎フレーム呼ぶ（ImGui の後、描く前）。選択に合わせて動かすものを決め、時間を進める
	static void Update();

	// Scene ビューの右下に操作パネルを描く。sceneRect は Scene ビューの絵の範囲（スクリーン座標）
	static void DrawPanel(const Rect &sceneRect);


private: // インナークラス

	enum class State : uint8_t {
		Stopped,
		Playing,
		Paused,
	};


private: // 静的メンバ変数

	static inline uint32_t sRootId_ = 0;
	static inline std::vector<uint32_t> sSystemIds_;

	static inline State sState_ = State::Stopped;
	static inline float sPlaybackSpeed_ = 1.0f;
	static inline float sPlaybackTime_ = 0.0f;


private: // 静的メンバ関数

	static GameObject *FindRoot(GameObject *selected);
	static void CollectSystems(GameObject *gameObject);
	static std::vector<ParticleSystem *> GetSystems();

	static void Play();
	static void Pause();
	static void Restart();
	static void Stop();

	// 粒の数・位置を Renderer の描く用のバッファに入れる（Play 中は LateUpdate がやっていること）
	static void RefreshRenderers();
};

} // namespace ou
