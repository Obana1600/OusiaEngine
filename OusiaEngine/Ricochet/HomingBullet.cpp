#include "HomingBullet.h"
#include "GameAssets.h"
#include "Math/Mathf.h"
#include "Player.h"
#include "Render/MeshRenderer.h"
#include "Scene/GameObject.h"
#include "Tuning.h"

#include <algorithm>

using namespace ou;



HomingBullet *ou::HomingBullet::Spawn(Scene &scene, const Vector2 &position, const Vector2 &velocity, float turnRate, float homingTime) {
	auto *renderer = CreateMeshObject(scene, "Homing Bullet", GameAssets::GetInstance()->GetSphere(), Vector3::Zero,
									  Vector3::One, {0.75f, 0.35f, 1.0f, 1.0f}, true);
	auto *bullet = renderer->GetGameObject()->AddComponent<HomingBullet>();
	bullet->Setup(position, velocity, Tuning::kHomingBulletRadius, Tuning::kHomingBulletDamage);
	bullet->turnRate_ = turnRate;
	bullet->homingTime_ = homingTime;
	bullet->lifetime_ = homingTime + 4.0f;
	bullet->AttachLight({0.75f, 0.35f, 1.0f, 1.0f}, 1.5f, 3.5f);
	return bullet;
}


bool ou::HomingBullet::Move(float dt) {
	const auto *player = Player::GetInstance();
	if (player && age_ < homingTime_) {
		// 向きの差を、旋回の上限までだけ縮める
		const auto toPlayer = player->GetPosition() - position_;
		const auto current = Mathf::Atan2(velocity_.y, velocity_.x);
		const auto target = Mathf::Atan2(toPlayer.y, toPlayer.x);
		auto delta = Mathf::Fmod(target - current + 3.0f * Mathf::PI, 2.0f * Mathf::PI) - Mathf::PI;
		const auto maxTurn = turnRate_ * dt;
		delta = std::clamp(delta, -maxTurn, maxTurn);

		const auto speed = velocity_.Length();
		const auto angle = current + delta;
		velocity_ = Vector2{Mathf::Cos(angle), Mathf::Sin(angle)} * speed;
	}

	position_ += velocity_ * dt;
	return !IsOutsideArena();
}
