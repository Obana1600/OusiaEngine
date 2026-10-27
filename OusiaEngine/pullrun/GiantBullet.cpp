#include "GiantBullet.h"
#include "GameAssets.h"
#include "GameAudio.h"
#include "GameColors.h"
#include "ParticleEffects.h"
#include "Render/MeshRenderer.h"
#include "Scene/GameObject.h"
#include "Tuning.h"

using namespace ou;



GiantBullet *ou::GiantBullet::Spawn(Scene &scene, const Vector2 &position, const Vector2 &velocity, float radius) {
	GameAudio::PlaySe(Se::BossBigBullet);
	auto *renderer = CreateMeshObject(scene, "Giant Bullet", GameAssets::GetInstance()->GetSphere(), Vector3::Zero,
									  Vector3::One, {1.0f, 0.5f, 0.1f, 1.0f}, false);
	auto *bullet = renderer->GetGameObject()->AddComponent<GiantBullet>();
	bullet->Setup(position, velocity, radius, Tuning::kGiantBulletDamage);
	bullet->lifetime_ = 15.0f;
	bullet->AttachLight({1.0f, 0.5f, 0.1f, 1.0f}, 4.0f, 7.0f);
	return bullet;
}


bool ou::GiantBullet::Move(float dt) {
	MotionParams params;
	params.radius = radius_;
	MotionState state = {position_, velocity_};
	const auto result = Motion2D::Step(state, params, dt);
	bounceCount_ += result.bounceCount;
	position_ = state.position;
	velocity_ = state.velocity;
	if (result.bounceCount > 0) {
		ParticleEffects::PlaySparks(position_ - result.lastWallNormal * radius_, result.lastWallNormal, GameColors::kGiantBullet, 12);
	}

	// 反射しきって消えるときは、はじけて消える
	if (bounceCount_ > Tuning::kGiantBulletMaxBounces) {
		ParticleEffects::PlayExplosion(position_, GameColors::kGiantBullet, radius_ * 0.5f);
		return false;
	}
	return true;
}
