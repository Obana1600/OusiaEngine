#pragma once

#include "Math/Color.h"
#include "Math/Quaternion.h"
#include "Math/Vector2.h"
#include "Math/Vector3.h"
#include "Scene/Component.h"
#include "SpeedBand.h"

#include <memory>


namespace ou {

// 前方宣言
class Light;
class Material;
class Player;
class Transform;

// プレイヤーの見た目。箱と、速度帯を示すオーラ。回転は当たり判定に影響しない。
// 箱は Rigidbody のように、ぶつかったときの力積でだけ回り始め、角速度の減衰で止まる（発射は中心を押すので回らない）
class PlayerVisual final: public Component {
public: // メンバ関数

	void Start() override;
	void LateUpdate() override;

	// normal はぶつかった相手からプレイヤーへの向き、impulse は速度の変化（質量 1 の力積）
	void OnImpact(const Vector2 &normal, const Vector2 &impulse);
	void OnBandChanged(SpeedBand band);
	void OnHit();


private: // メンバ変数

	Player *player_ = nullptr;
	Transform *body_ = nullptr;
	Transform *aura_ = nullptr;
	// 速度帯の色で周りを照らす
	Light *light_ = nullptr;
	std::shared_ptr<Material> bodyMaterial_;
	std::shared_ptr<Material> auraMaterial_;

	Quaternion spin_ = Quaternion::Identity;
	Vector3 angularVelocity_;
	// 中速帯に入った瞬間などに光らせる
	float flashTimer_ = 0.0f;
	float hitFlashTimer_ = 0.0f;


private: // メンバ関数

	Color ComputeBodyColor(SpeedBand band) const;
};

} // namespace ou
