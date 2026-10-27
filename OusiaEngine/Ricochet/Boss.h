#pragma once

#include "BossAction.h"
#include "Math/Color.h"
#include "Math/Vector2.h"
#include "Motion2D.h"
#include "Scene/Component.h"

#include <cstdint>
#include <memory>


namespace ou {

// 前方宣言
class BossWeakPoint;
class Light;
class Material;
class Scene;

enum class BossState : uint8_t {
	Fighting,
	// 第一形態 → 第二形態の移行中（無敵）
	Transition,
	Defeated,
};

// ボス。反射で狙いを定められるよう基本は静止し、行動をランダムに選ぶ
class Boss final: public Component {
public: // 静的メンバ関数

	static Boss *GetInstance() { return sInstance_; }


public: // メンバ関数

	void Awake() override;
	void Start() override;
	void Update() override;
	void LateUpdate() override;
	void OnDestroy() override;

	Vector2 GetPosition() const { return position_; }
	void SetPosition(const Vector2 &position) { position_ = position; }
	float GetRadius() const;
	int GetPhase() const { return phase_; }
	BossState GetState() const { return state_; }
	float GetHp() const { return hp_; }
	float GetMaxHp() const;
	Scene &GetScene() const;

	// ダメージを受けられるか（移行中・撃破後・多段ヒット防止中は受けない）
	bool CanTakeDamage() const;
	// direction はボスの中心から当たった位置への向き。弱点に当たると大きく増える。与えたダメージを返す
	float TakeDamage(float amount, const Vector2 &direction);
	bool IsWeakPointHit(const Vector2 &direction) const;

	// 反射突進
	bool IsDashing() const { return isDashing_; }
	void SetDashing(bool isDashing) { isDashing_ = isDashing; }
	Vector2 GetDashVelocity() const { return dashVelocity_; }
	void SetDashVelocity(const Vector2 &velocity) { dashVelocity_ = velocity; }
	// 突進中に高速のプレイヤーとぶつかって反射する
	void ReflectDash(const Vector2 &normal);

	// 予兆として体を光らせる
	void SetWarningGlow(float amount) { warningGlow_ = amount; }
	float GetWarningGlow() const { return warningGlow_; }


private: // 静的メンバ変数

	static inline Boss *sInstance_ = nullptr;


private: // メンバ変数

	Vector2 position_;
	float hp_ = 0.0f;
	int phase_ = 1;
	BossState state_ = BossState::Fighting;
	float stateTimer_ = 0.0f;

	std::unique_ptr<BossAction> action_;
	int lastActionIndex_ = -1;
	float idleTimer_ = 1.5f;

	bool isDashing_ = false;
	Vector2 dashVelocity_;
	float hitCooldown_ = 0.0f;
	float damageFlash_ = 0.0f;
	float warningGlow_ = 0.0f;

	BossWeakPoint *weakPoint_ = nullptr;
	std::shared_ptr<Material> bodyMaterial_;
	// 形態の色で周りを照らし、攻撃の予兆で強く光る
	Light *light_ = nullptr;


private: // メンバ関数

	void UpdateFighting(float dt);
	void ChooseNextAction();
	void CancelAction();
	void BeginTransition();
	Color ComputeBodyColor() const;
};

} // namespace ou
