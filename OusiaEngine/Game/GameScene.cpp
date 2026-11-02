#include "GameScene.h"
#include "Diagnostics/Log.h"
#include "Physics/BoxCollider.h"
#include "Physics/Physics.h"
#include "Physics/Rigidbody.h"
#include "Render/Camera.h"
#include "Scene/GameObject.h"
#include "Scene/Transform.h"

#include <string>

using namespace ou;


namespace {

GameObject *MakeBox(Scene &scene, const char *name, const Vector3 &position, const Vector3 &size) {
	auto *object = scene.CreateGameObject(name);
	object->GetTransform()->SetWorldPosition(position);
	object->AddComponent<BoxCollider>()->SetSize(size);
	return object;
}

void LogHits(const char *label, const std::vector<RaycastHit> &hits) {
	std::string text;
	for (const auto &hit : hits) {
		text += std::format(" {}({:.3f} p ({:.2f}, {:.2f}, {:.2f}) n ({:.2f}, {:.2f}, {:.2f}) t {})",
							hit.collider->GetGameObject()->GetName(), hit.distance, hit.point.x, hit.point.y, hit.point.z,
							hit.normal.x, hit.normal.y, hit.normal.z, hit.GetTransform()->GetGameObject()->GetName());
	}
	Log::Info("TEST {}:{}", label, hits.empty() ? " (なし)" : text);
}

void LogColliders(const char *label, const std::vector<Collider *> &colliders) {
	std::string text;
	for (const auto *collider : colliders) {
		text += " " + collider->GetGameObject()->GetName();
	}
	Log::Info("TEST {}:{}", label, colliders.empty() ? " (なし)" : text);
}

} // namespace


void ou::BuildGameScene(Scene &scene) {
	scene.CreateGameObject("Probe")->AddComponent<QueryProbe>();
}


void ou::QueryProbe::Start() {
	auto &scene = *GetGameObject()->GetScene();
	scene.CreateGameObject("Main Camera")->AddComponent<Camera>();

	MakeBox(scene, "Floor", {0, 0, 0}, {200, 1, 60});
	MakeBox(scene, "BoxA", {0, 2, 0}, {1, 1, 1});
	MakeBox(scene, "BoxB", {0, 4, 0}, {1, 1, 1});
	MakeBox(scene, "Trigger", {0, 7, 0}, {2, 0.2f, 2})->GetComponent<BoxCollider>()->SetIsTrigger(true);

	// Rigidbody の子のコライダー
	auto *parent = scene.CreateGameObject("Parent");
	parent->GetTransform()->SetWorldPosition({6, 2, 0});
	parent->AddComponent<Rigidbody>()->SetIsKinematic(true);
	auto *child = scene.CreateGameObject("Child");
	child->GetTransform()->SetParent(parent->GetTransform());
	child->AddComponent<BoxCollider>();

	// 無効にしたコライダー
	MakeBox(scene, "Off", {10, 2, 0}, {1, 1, 1})->GetComponent<BoxCollider>()->SetEnabled(false);
}


void ou::QueryProbe::FixedUpdate() {
	// 形は最初の Simulate で作られるので、2 歩目に調べる
	if (++step_ != 2) { return; }

	const Vector3 down = {0, -1, 0};
	LogHits("上から", Physics::RaycastAll({0, 10, 0}, down));
	LogHits("向きの長さ 5", Physics::RaycastAll({0, 10, 0}, down * 5.0f));
	LogHits("Ignore", Physics::RaycastAll({0, 10, 0}, down, Mathf::Infinity, QueryTriggerInteracation::Ignore));
	Physics::SetQueriesHitTriggers(false);
	LogHits("UseGlobal（queriesHitTriggers false）", Physics::RaycastAll({0, 10, 0}, down));
	Physics::SetQueriesHitTriggers(true);
	LogHits("maxDistance 3・Ignore", Physics::RaycastAll({0, 10, 0}, down, 3.0f, QueryTriggerInteracation::Ignore));
	LogHits("maxDistance 6・Ignore", Physics::RaycastAll({0, 10, 0}, down, 6.0f, QueryTriggerInteracation::Ignore));
	LogHits("始点が BoxB の中", Physics::RaycastAll({0, 4, 0}, down));
	LogHits("横から", Physics::RaycastAll({-5, 2, 0}, {1, 0, 0}, 10.0f));
	LogHits("Rigidbody の子", Physics::RaycastAll({6, 10, 0}, down, 20.0f));
	LogHits("無効にしたコライダー", Physics::RaycastAll({10, 10, 0}, down, 20.0f));

	RaycastHit hit;
	const auto isHit = Physics::Raycast({0, 10, 0}, down, &hit, Mathf::Infinity, QueryTriggerInteracation::Ignore);
	Log::Info("TEST Raycast（Ignore）: {} {} {:.3f}", isHit, isHit ? hit.collider->GetGameObject()->GetName() : "", hit.distance);
	Log::Info("TEST Raycast（何もない方向）: {}", Physics::Raycast({0, 10, 0}, {0, 1, 0}));

	LogColliders("OverlapSphere (0, 2, 0) r 0.6", Physics::OverlapSphere({0, 2, 0}, 0.6f));
	LogColliders("OverlapSphere (0, 3, 0) r 1", Physics::OverlapSphere({0, 3, 0}, 1.0f));
	LogColliders("OverlapSphere Trigger・Ignore", Physics::OverlapSphere({0, 7, 0}, 0.5f, QueryTriggerInteracation::Ignore));
	LogColliders("OverlapSphere Trigger・Collide", Physics::OverlapSphere({0, 7, 0}, 0.5f, QueryTriggerInteracation::Collide));
	LogColliders("OverlapBox z に長い", Physics::OverlapBox({2, 2, 0}, {0.3f, 0.3f, 3}));
	LogColliders("OverlapBox y で 90 度", Physics::OverlapBox({2, 2, 0}, {0.3f, 0.3f, 3}, Quaternion::Euler(0, 90, 0)));
	LogColliders("OverlapCapsule 横", Physics::OverlapCapsule({-2, 2, 0}, {2, 2, 0}, 0.3f));
	LogColliders("OverlapCapsule 縦", Physics::OverlapCapsule({0, 2, 0}, {0, 4, 0}, 0.2f));
	LogColliders("OverlapCapsule 離れている", Physics::OverlapCapsule({3, 2, 0}, {3, 4, 0}, 0.2f));
}
