#include "ParticleEmitter.h"
#include "Math/Mathf.h"
#include "Math/Quaternion.h"
#include "Render/Asset/Material.h"
#include "Scene/GameObject.h"
#include "Scene/Scene.h"
#include "Scene/Transform.h"

using namespace ou;



ParticleSystem *ou::ParticleEmitter::Create(Scene &scene, const EmitterSettings &settings) {
	auto *object = scene.CreateGameObject(settings.name);
	// ParticleSystemRenderer は ParticleSystem も付ける
	auto *renderer = object->AddComponent<ParticleSystemRenderer>();
	renderer->SetBlendMode(settings.blendMode);
	if (settings.texture) { renderer->GetMaterial()->SetMainTexture(settings.texture); }

	auto *particleSystem = object->GetComponent<ParticleSystem>();
	particleSystem->Stop();
	auto &main = particleSystem->GetMain();
	main.playOnAwake = false;
	main.simulationSpace = settings.simulationSpace;
	main.maxParticles = settings.maxParticles;
	return particleSystem;
}


void ou::ParticleEmitter::Aim(ParticleSystem &emitter, const Vector2 &position, const Vector2 &direction, float z) {
	// 円錐はローカルの +y 向きに出るので、+y が direction を向くよう z 軸まわりに回す
	auto *transform = emitter.GetTransform();
	transform->SetLocalPosition({position.x, position.y, z});
	transform->SetLocalRotation(Quaternion::Euler(0.0f, 0.0f, Mathf::Atan2(direction.y, direction.x) - Mathf::PI * 0.5f));
}


void ou::ParticleEmitter::AimRing(ParticleSystem &emitter, const Vector2 &position, float z) {
	// 角度 90 度の円錐はローカルの xz 平面に広がるので、x 軸まわりに 90 度回して xy 平面にする
	auto *transform = emitter.GetTransform();
	transform->SetLocalPosition({position.x, position.y, z});
	transform->SetLocalRotation(Quaternion::Euler(Mathf::PI * 0.5f, 0.0f, 0.0f));
}
