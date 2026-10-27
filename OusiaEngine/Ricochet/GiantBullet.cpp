#include "GiantBullet.h"
#include "GameAssets.h"
#include "Render/MeshRenderer.h"
#include "Scene/GameObject.h"
#include "Tuning.h"

using namespace ou;



GiantBullet *ou::GiantBullet::Spawn(Scene &scene, const Vector2 &position, const Vector2 &velocity, float radius) {
	auto *renderer = CreateMeshObject(scene, "Giant Bullet", GameAssets::GetInstance()->GetSphere(), Vector3::Zero,
									  Vector3::One, {1.0f, 0.5f, 0.1f, 1.0f}, false);
	auto *bullet = renderer->GetGameObject()->AddComponent<GiantBullet>();
	bullet->Setup(position, velocity, radius, Tuning::kGiantBulletDamage);
	bullet->lifetime_ = 15.0f;
	return bullet;
}


bool ou::GiantBullet::Move(float dt) {
	MotionParams params;
	params.radius = radius_;
	MotionState state = {position_, velocity_};
	bounceCount_ += Motion2D::Step(state, params, dt).bounceCount;
	position_ = state.position;
	velocity_ = state.velocity;
	return bounceCount_ <= Tuning::kGiantBulletMaxBounces;
}
