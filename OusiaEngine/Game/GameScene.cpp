#include "GameScene.h"
#include "Core/Time.h"
#include "Diagnostics/Log.h"
#include "Physics/BoxCollider.h"
#include "Physics/Collision.h"
#include "Physics/Physics.h"
#include "Physics/Rigidbody.h"
#include "Physics/SphereCollider.h"
#include "Render/Camera.h"
#include "Scene/GameObject.h"
#include "Scene/LayerMask.h"
#include "Scene/Transform.h"

#include <string>

using namespace ou;


namespace {

GameObject *CreateBox(Scene &scene, const char *name, const char *layer, const Vector3 &position, const Vector3 &size, bool dynamic) {
	auto *object = scene.CreateGameObject(name);
	object->SetLayer(LayerMask::NameToLayer(layer));
	object->GetTransform()->SetWorldPosition(position);
	object->AddComponent<BoxCollider>()->SetSize(size);
	if (dynamic) { object->AddComponent<Rigidbody>(); }
	object->AddComponent<ContactLogger>();
	return object;
}

GameObject *CreateBall(Scene &scene, const char *name, const char *layer, const Vector3 &position) {
	auto *object = scene.CreateGameObject(name);
	object->SetLayer(LayerMask::NameToLayer(layer));
	object->GetTransform()->SetWorldPosition(position);
	object->AddComponent<SphereCollider>()->SetRadius(0.3f);
	object->AddComponent<Rigidbody>();
	object->AddComponent<ContactLogger>();
	return object;
}

std::string Names(const std::vector<Collider *> &colliders) {
	std::string names;
	for (auto *collider : colliders) {
		names += " " + collider->GetGameObject()->GetName();
	}
	return names.empty() ? " (なし)" : names;
}

std::string RayResult(bool hit, const RaycastHit &hitInfo) {
	if (!hit) { return "(なし)"; }
	return std::format("{} {:.2f}", hitInfo.collider->GetGameObject()->GetName(), hitInfo.distance);
}

} // namespace



void ou::BuildGameScene(Scene &scene) {
	scene.CreateGameObject("Main Camera")->AddComponent<Camera>();

	// Player と Enemy、Ghost と Ground は当たらない
	Physics::IgnoreLayerCollision(LayerMask::NameToLayer("Player"), LayerMask::NameToLayer("Enemy"));
	Physics::IgnoreLayerCollision(LayerMask::NameToLayer("Ghost"), LayerMask::NameToLayer("Ground"));

	CreateBox(scene, "Floor", "Ground", {0.0f, -0.5f, 0.0f}, {40.0f, 1.0f, 40.0f}, false);

	auto *probe = scene.CreateGameObject("Probe")->AddComponent<FilterProbe>();

	// Enemy の箱は Player の箱をすり抜けて床に落ちる
	probe->playerBox = CreateBox(scene, "PlayerBox", "Player", {0.0f, 0.5f, 0.0f}, Vector3::One, true);
	probe->enemyBox = CreateBox(scene, "EnemyBox", "Enemy", {0.0f, 3.0f, 0.0f}, Vector3::One, true);

	// Enemy のトリガーに、Player の玉は入らず、Default の玉は入る
	CreateBox(scene, "EnemyZone", "Enemy", {4.0f, 2.0f, 0.0f}, {2.0f, 1.0f, 8.0f}, false)->GetComponent<BoxCollider>()->SetIsTrigger(true);
	probe->ghost = CreateBall(scene, "PlayerBall", "Player", {4.0f, 5.0f, -2.0f});
	probe->visitor = CreateBall(scene, "DefaultBall", "Default", {4.0f, 5.0f, 2.0f});

	// 3 秒で Default と Ground を当たらなくする / 2 秒で Ghost に変える
	probe->sleeper = CreateBox(scene, "Sleeper", "Default", {-4.0f, 0.5f, 0.0f}, Vector3::One, true);
	probe->switcher = CreateBox(scene, "Switcher", "Player", {-8.0f, 0.5f, 0.0f}, Vector3::One, true);

	// Ignore Raycast の箱（床の上、y 0.5〜1.5）
	CreateBox(scene, "NoRay", "Ignore Raycast", {8.0f, 1.0f, 0.0f}, Vector3::One, false);
}


void ou::ContactLogger::OnCollisionEnter(const Collision &collision) {
	Log::Info("TEST CollisionEnter {} <- {}", GetGameObject()->GetName(), collision.GetGameObject()->GetName());
}


void ou::ContactLogger::OnCollisionExit(const Collision &collision) {
	Log::Info("TEST CollisionExit {} <- {}", GetGameObject()->GetName(), collision.GetGameObject()->GetName());
}


void ou::ContactLogger::OnTriggerEnter(Collider *other) {
	Log::Info("TEST TriggerEnter {} <- {}", GetGameObject()->GetName(), other->GetGameObject()->GetName());
}


void ou::ContactLogger::OnTriggerExit(Collider *other) {
	Log::Info("TEST TriggerExit {} <- {}", GetGameObject()->GetName(), other->GetGameObject()->GetName());
}


void ou::FilterProbe::Start() {
	const auto player = LayerMask::NameToLayer("Player");
	const auto enemy = LayerMask::NameToLayer("Enemy");
	const auto ground = LayerMask::NameToLayer("Ground");
	Log::Info("TEST Get Player-Enemy {} Enemy-Player {} Player-Ground {} Ghost-Ground {}",
			  Physics::GetIgnoreLayerCollision(player, enemy), Physics::GetIgnoreLayerCollision(enemy, player),
			  Physics::GetIgnoreLayerCollision(player, ground), Physics::GetIgnoreLayerCollision(LayerMask::NameToLayer("Ghost"), ground));
	Physics::IgnoreLayerCollision(40, 0);
	Log::Info("TEST Get 40-0 {} (範囲の外は false)", Physics::GetIgnoreLayerCollision(40, 0));
	Log::Info("TEST masks Default 0x{:08X} IgnoreRaycast 0x{:08X} All 0x{:08X}",
			  Physics::kDefaultRaycastLayers, Physics::kIgnoreRaycastLayer, Physics::kAllLayers);
}


void ou::FilterProbe::Update() {
	const auto time = Time::GetTime();
	if (step_ == 0 && time >= 1.0f) {
		step_++;
		LogQueries();
	} else if (step_ == 1 && time >= 1.9f) {
		step_++;
		LogPositions("1.9s");
		switcher->SetLayer(LayerMask::NameToLayer("Ghost"));
		Log::Info("TEST 2s Switcher を Ghost に");
	} else if (step_ == 2 && time >= 3.0f) {
		step_++;
		Physics::IgnoreLayerCollision(LayerMask::NameToLayer("Default"), LayerMask::NameToLayer("Ground"));
		Log::Info("TEST 3s Default と Ground を当たらなくする");
	} else if (step_ == 3 && time >= 4.5f) {
		step_++;
		LogPositions("4.5s");
	}
}


void ou::FilterProbe::LogPositions(const char *label) const {
	for (auto *object : {playerBox, enemyBox, ghost, visitor, sleeper, switcher}) {
		Log::Info("TEST {} {} y {:.2f}", label, object->GetName(), object->GetTransform()->GetWorldPosition().y);
	}
}


void ou::FilterProbe::LogQueries() const {
	const Vector3 origin = {8.0f, 10.0f, 0.0f};
	const Vector3 down = {0.0f, -1.0f, 0.0f};
	RaycastHit hit;
	bool found = Physics::Raycast(origin, down, &hit);
	Log::Info("TEST Raycast 既定 {}", RayResult(found, hit));
	found = Physics::Raycast(origin, down, &hit, Mathf::Infinity, Physics::kAllLayers);
	Log::Info("TEST Raycast All {}", RayResult(found, hit));
	found = Physics::Raycast(origin, down, &hit, Mathf::Infinity, LayerMask::GetMask({"Player"}));
	Log::Info("TEST Raycast Player {}", RayResult(found, hit));
	Log::Info("TEST RaycastAll All 件数 {}", Physics::RaycastAll(origin, down, Mathf::Infinity, Physics::kAllLayers).size());

	const Vector3 center = {8.0f, 1.0f, 0.0f};
	Log::Info("TEST OverlapSphere 既定{}", Names(Physics::OverlapSphere(center, 0.3f)));
	Log::Info("TEST OverlapSphere DefaultRaycastLayers{}", Names(Physics::OverlapSphere(center, 0.3f, Physics::kDefaultRaycastLayers)));
	Log::Info("TEST OverlapBox Ground{}", Names(Physics::OverlapBox({0.0f, 0.0f, 0.0f}, {1.0f, 1.2f, 1.0f}, Quaternion::Identity, LayerMask::GetMask({"Ground"}))));
	Log::Info("TEST OverlapBox Player+Enemy{}", Names(Physics::OverlapBox({0.0f, 0.0f, 0.0f}, {1.0f, 1.2f, 1.0f}, Quaternion::Identity, LayerMask::GetMask({"Player", "Enemy"}))));
	Log::Info("TEST OverlapCapsule なし{}", Names(Physics::OverlapCapsule({0.0f, 0.0f, 0.0f}, {0.0f, 2.0f, 0.0f}, 0.5f, 0)));
}
