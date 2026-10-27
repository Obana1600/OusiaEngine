#pragma once

#include "Math/Color.h"
#include "Math/Vector2.h"
#include "Scene/Component.h"

#include <cstdint>
#include <vector>


namespace ou {

enum class ProjectileKind : uint8_t {
	Small,
	Homing,
	Giant,
};

// ボスが撃つ弾の共通部分。当たり判定は球
class Projectile: public Component {
public: // 静的メンバ関数

	static const std::vector<Projectile *> &GetAll() { return sAll_; }
	static int CountOf(ProjectileKind kind);
	// 第二形態への移行などで、すべての弾を消す
	static void DestroyAll();


public: // メンバ関数

	void Awake() override;
	void Update() override;
	void OnDestroy() override;

	virtual ProjectileKind GetKind() const = 0;

	Vector2 GetPosition() const { return position_; }
	Vector2 GetVelocity() const { return velocity_; }
	float GetRadius() const { return radius_; }
	float GetDamage() const { return damage_; }

	void Vanish();


protected: // メンバ変数

	Vector2 position_;
	Vector2 velocity_;
	float radius_ = 0.3f;
	float damage_ = 0.0f;
	float age_ = 0.0f;
	float lifetime_ = 10.0f;


protected: // メンバ関数

	void Setup(const Vector2 &position, const Vector2 &velocity, float radius, float damage);
	// 点光源を付ける。ライトの数には上限があるので、弾に回せる数を超えたら付けない
	void AttachLight(const Color &color, float intensity, float range);
	// dt だけ動かす。消えるなら false
	virtual bool Move(float dt) = 0;
	bool IsOutsideArena() const;


private: // 静的メンバ変数

	static inline std::vector<Projectile *> sAll_;
	static inline int sLitCount_ = 0;


private: // メンバ変数

	bool isVanished_ = false;
	bool hasLight_ = false;
};

} // namespace ou
