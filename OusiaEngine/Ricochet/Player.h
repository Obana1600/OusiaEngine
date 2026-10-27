#pragma once

#include "Motion2D.h"
#include "PullInput.h"
#include "Scene/Component.h"
#include "SpeedBand.h"


namespace ou {

// 前方宣言
class PlayerVisual;

// 当たり判定は球。移動は発射だけで、壁で反射しながら減速する
class Player final: public Component {
public: // 静的メンバ関数

	static Player *GetInstance() { return sInstance_; }
	static MotionParams MakeMotionParams();


public: // メンバ関数

	void Awake() override;
	void Start() override;
	void Update() override;
	void OnDestroy() override;

	Vector2 GetPosition() const { return motion_.position; }
	void SetPosition(const Vector2 &position) { motion_.position = position; }
	Vector2 GetVelocity() const { return motion_.velocity; }
	void SetVelocity(const Vector2 &velocity) { motion_.velocity = velocity; }
	float GetSpeed() const { return motion_.velocity.Length(); }
	float GetRadius() const;
	SpeedBand GetBand() const { return ClassifySpeed(GetSpeed()); }
	const MotionState &GetMotion() const { return motion_; }

	const PullInput &GetPull() const { return pull_; }
	// 今の溜めで発射したときの速度
	Vector2 ComputeLaunchVelocity() const;

	// 被弾。無敵中なら false。低速なら吹っ飛び、中速以上ならアーマーで受ける
	bool TakeHit(float damage, const Vector2 &knockbackDirection);
	bool IsInvincible() const { return invincibleTimer_ > 0.0f; }
	bool IsDead() const { return hp_ <= 0.0f; }
	float GetHp() const { return hp_; }

	int GetStock() const { return stock_; }
	void AddStock(int count);

	// ボスや巨大弾に当たって跳ね返る
	void Bounce(const Vector2 &normal, float restitution);

	// 時間を止めて操作を受け付けない（クリア・ゲームオーバー）
	void Freeze();


private: // 静的メンバ変数

	static inline Player *sInstance_ = nullptr;


private: // メンバ変数

	MotionState motion_;
	PullInput pull_;
	PlayerVisual *visual_ = nullptr;

	float hp_ = 0.0f;
	int stock_ = 0;
	float stockTimer_ = 0.0f;
	float invincibleTimer_ = 0.0f;
	SpeedBand lastBand_ = SpeedBand::Low;
	bool isFrozen_ = false;


private: // メンバ関数

	void Launch();
};

} // namespace ou
