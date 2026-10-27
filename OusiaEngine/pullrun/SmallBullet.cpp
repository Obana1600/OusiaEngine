#include "SmallBullet.h"
#include "GameAssets.h"
#include "GameAudio.h"
#include "Render/MeshRenderer.h"
#include "Scene/GameObject.h"
#include "Tuning.h"

using namespace ou;



SmallBullet *ou::SmallBullet::Spawn(Scene &scene, const Vector2 &position, const Vector2 &velocity) {
	GameAudio::PlaySe(Se::BossSmallBullet);
	auto *renderer = CreateMeshObject(scene, "Small Bullet", GameAssets::GetInstance()->GetSphere(), Vector3::Zero,
									  Vector3::One, {1.0f, 0.35f, 0.5f, 1.0f}, true);
	auto *bullet = renderer->GetGameObject()->AddComponent<SmallBullet>();
	bullet->Setup(position, velocity, Tuning::kSmallBulletRadius, Tuning::kSmallBulletDamage);
	return bullet;
}


bool ou::SmallBullet::Move(float dt) {
	position_ += velocity_ * dt;
	return !IsOutsideArena();
}
