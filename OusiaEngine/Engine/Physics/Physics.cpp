#include "Physics.h"
#include "Core/JsonFile.h"
#include "Core/StringUtil.h"
#include "Core/Time.h"
#include "Diagnostics/Log.h"
#include "Physics/Box3D/PhysicsWorld.h"
#include "Physics/CharacterController.h"
#include "Physics/Collider.h"
#include "Physics/Collision.h"
#include "Physics/Rigidbody.h"
#include "Platform/Paths.h"
#include "Scene/GameObject.h"

#include <algorithm>
#include <filesystem>
#include <string_view>
#include <unordered_map>
#include <utility>

using namespace ou;


namespace {

constexpr std::string_view kDynamicsManagerKeys[] = {
	"gravity",
	"queriesHitTriggers",
	"ignoreLayerCollisions",
};

void LoadGravity(const Json &json, const std::string &pathUtf8) {
	const auto it = json.find("gravity");
	if (it == json.end()) { return; }

	const auto isVector3 = it->is_array() && it->size() == 3 &&
						   std::all_of(it->begin(), it->end(), [](const auto &value) { return value.is_number(); });
	if (!isVector3) {
		Log::Warning("\"gravity\" は [x, y, z] で書いてください: {}", pathUtf8);
		return;
	}

	Physics::SetGravity({(*it)[0].get<float>(), (*it)[1].get<float>(), (*it)[2].get<float>()});
}

void LoadQueriesHitTriggers(const Json &json, const std::string &pathUtf8) {
	const auto it = json.find("queriesHitTriggers");
	if (it == json.end()) { return; }

	if (!it->is_boolean()) {
		Log::Warning("\"queriesHitTriggers\" は true か false で書いてください: {}", pathUtf8);
		return;
	}

	Physics::SetQueriesHitTriggers(it->get<bool>());
}

void LoadIgnoreLayerCollisions(const Json &json, const std::string &pathUtf8) {
	const auto it = json.find("ignoreLayerCollisions");
	if (it == json.end()) { return; }

	if (!it->is_array()) {
		Log::Warning("\"ignoreLayerCollisions\" は [[\"名前\", \"名前\"], ...] で書いてください: {}", pathUtf8);
		return;
	}

	for (const auto &pair : *it) {
		const auto isNamePair = pair.is_array() && pair.size() == 2 &&
								pair[0].is_string() && pair[1].is_string();
		if (!isNamePair) {
			Log::Warning("\"ignoreLayerCollisions\" は [[\"名前\", \"名前\"], ...] で書いてください: {}", pathUtf8);
			continue;
		}

		const auto layer1 = LayerMask::NameToLayer(pair[0].get<std::string>());
		const auto layer2 = LayerMask::NameToLayer(pair[1].get<std::string>());
		if (layer1 < 0 || layer2 < 0) {
			Log::Warning("TagManager にないレイヤーの名前です: {}: {}", pathUtf8, pair.dump());
			continue;
		}

		Physics::IgnoreLayerCollision(layer1, layer2);
	}
}

template <typename Message>
Message SelectMessage(PhysicsWorld::ContactEvent::Type type, Message enter, Message stay, Message exit) {
	switch (type) {
		case PhysicsWorld::ContactEvent::Type::Enter:
			return enter;
		case PhysicsWorld::ContactEvent::Type::Stay:
			return stay;
		default:
			return exit;
	}
}

Collision MakeCollision(const PhysicsWorld::ContactEvent &event, Collider *self, Collider *other, const Vector3 &relativeVelocity, bool isShapeA) {
	const auto normal = isShapeA ? -event.normal : event.normal;
	const auto impulse = isShapeA ? -event.impulse : event.impulse;

	std::vector<ContactPoint> contacts;
	for (const auto &point : event.points) {
		contacts.push_back({point, normal, event.separation, self, other});
	}
	return Collision(other, other->GetAttachedRigidbody(), std::move(contacts), relativeVelocity, impulse);
}

bool IsValidLayer(int layer) {
	return layer >= 0 && layer < LayerMask::kLayerCount;
}

bool CanReceiveTrigger(const Collider *collider) {
	return collider->GetAttachedRigidbody() || dynamic_cast<const CharacterController *>(collider);
}

} // namespace



void ou::Physics::Simulate() {
	// 剛体の更新
	SyncTransform();
	for (auto *rigidbody : sRigidbodies_) {
		rigidbody->ClampVelocity();
		rigidbody->StoreVelocityBeforeStep();
	}
	PhysicsWorld::Step(Time::GetDeltaTime());
	for (auto *rigidbody : sRigidbodies_) {
		rigidbody->ClampVelocity();
		rigidbody->WriteBackPose();
	}

	DispatchContactEvents();
}


void ou::Physics::Interpolate() {
	const auto elapsed = static_cast<float>(Time::GetTimeAsDouble() - Time::GetFixedTimeAsDouble());
	const auto alpha = elapsed / Time::GetFixedDeltaTime();

	for (auto *rigidbody : sRigidbodies_) {
		rigidbody->UpdateInterpolatedPose(alpha, elapsed);
	}
}


void ou::Physics::SyncTransform() {
	for (auto *rigidbody : sRigidbodies_) {
		rigidbody->SyncBodyPose();
	}
	for (auto *collider : sColliders_) {
		collider->SyncToPhysics();
	}

	sPendingColliders_.clear();
}


void ou::Physics::SetGravity(const Vector3 &gravity) {
	PhysicsWorld::SetGravity(gravity);
}


Vector3 ou::Physics::GetGravity() {
	return PhysicsWorld::GetGravity();
}


void ou::Physics::SetQueriesHitTriggers(bool value) {
	sQueryHitTriggers_ = value;
}


bool ou::Physics::GetQueriesHitTriggers() {
	return sQueryHitTriggers_;
}


void ou::Physics::IgnoreLayerCollision(int layer1, int layer2, bool ignore) {
	if (!IsValidLayer(layer1) || !IsValidLayer(layer2)) {
		Log::Error("レイヤーは 0~31 で指定してください: {}, {}", layer1, layer2);
		return;
	}

	const auto bit1 = 1u << layer1;
	const auto bit2 = 1u << layer2;
	if (ignore) {
		sIgnoreLayerMasks_[layer1] |= bit2;
		sIgnoreLayerMasks_[layer2] |= bit1;
	} else {
		sIgnoreLayerMasks_[layer1] &= ~bit2;
		sIgnoreLayerMasks_[layer2] &= ~bit1;
	}
}


bool ou::Physics::GetIgnoreLayerCollision(int layer1, int layer2) {
	if (!IsValidLayer(layer1) || !IsValidLayer(layer2)) { return false; }
	return (sIgnoreLayerMasks_[layer1] & (1u << layer2)) != 0;
}


void ou::Physics::LoadDynamicsManager() {
	const auto folder = FindProjectSettingsFolder();
	if (!folder) { return; }
	const auto path = *folder / L"DynamicsManager.json";
	if (!std::filesystem::exists(path)) { return; }

	const auto json = JsonFile::Load(path);
	if (!json) { return; }

	const auto pathUtf8 = ToUtf8(path);
	if (!json->is_object()) {
		Log::Warning("DynamicsManager は {{ }} で書いてください: {}", pathUtf8);
		return;
	}

	JsonFile::WarnUnknownKeys(*json, kDynamicsManagerKeys, pathUtf8);
	LoadGravity(*json, pathUtf8);
	LoadQueriesHitTriggers(*json, pathUtf8);
	LoadIgnoreLayerCollisions(*json, pathUtf8);
}


bool ou::Physics::Raycast(const Vector3 &origin, const Vector3 &direction, RaycastHit *hitInfo, float maxDistance, uint32_t layerMask, QueryTriggerInteracation queryTriggerInteraction) {
	const auto hits = RaycastAll(origin, direction, maxDistance, layerMask, queryTriggerInteraction);
	if (hits.empty()) { return false; }

	if (hitInfo) { *hitInfo = hits.front(); }
	return true;
}


std::vector<RaycastHit> ou::Physics::RaycastAll(const Vector3 &origin, const Vector3 &direction, float maxDistance, uint32_t layerMask, QueryTriggerInteracation queryTriggerInteraction) {
	SyncPendingColliders();

	std::vector<RaycastHit> hits;
	for (const auto &result : PhysicsWorld::RaycastAll(origin, direction, maxDistance, layerMask, HitsTriggers(queryTriggerInteraction))) {
		if (auto *collider = FindCollider(result.shapeId)) {
			hits.push_back({result.point, result.normal, result.distance, collider});
		}
	}
	return hits;
}


std::vector<Collider *> ou::Physics::OverlapBox(const Vector3 &center, const Vector3 &halfExtents, const Quaternion &orientation, uint32_t layerMask, QueryTriggerInteracation queryTriggerInteraction) {
	SyncPendingColliders();
	return FindColliders(PhysicsWorld::OverlapBox(center, halfExtents, orientation, layerMask, HitsTriggers(queryTriggerInteraction)));
}


std::vector<Collider *> ou::Physics::OverlapSphere(const Vector3 &center, float radius, uint32_t layerMask, QueryTriggerInteracation queryTriggerInteraction) {
	SyncPendingColliders();
	return FindColliders(PhysicsWorld::OverlapSphere(center, radius, layerMask, HitsTriggers(queryTriggerInteraction)));
}


std::vector<Collider *> ou::Physics::OverlapCapsule(const Vector3 &point0, const Vector3 &point1, float radius, uint32_t layerMask, QueryTriggerInteracation queryTriggerInteraction) {
	SyncPendingColliders();
	return FindColliders(PhysicsWorld::OverlapCapsule(point0, point1, radius, layerMask, HitsTriggers(queryTriggerInteraction)));
}


bool ou::Physics::BoxCast(const Vector3 &center, const Vector3 &halfExtents, const Vector3 &direction, const Quaternion &orientation, RaycastHit *hitInfo, float maxDistance, uint32_t layerMask, QueryTriggerInteracation queryTriggerInteraction) {
	return FindFirstCastHit(BoxCastAll(center, halfExtents, direction, orientation, maxDistance, layerMask, queryTriggerInteraction), hitInfo);
}


std::vector<RaycastHit> ou::Physics::BoxCastAll(const Vector3 &center, const Vector3 &halfExtents, const Vector3 &direction, const Quaternion &orientation, float maxDistance, uint32_t layerMask, QueryTriggerInteracation queryTriggerInteraction) {
	SyncPendingColliders();
	return ToCastHits(PhysicsWorld::BoxCastAll(center, halfExtents, orientation, direction, maxDistance, layerMask, HitsTriggers(queryTriggerInteraction)), direction);
}


bool ou::Physics::SphereCast(const Vector3 &origin, float radius, const Vector3 &direction, RaycastHit *hitInfo, float maxDistance, uint32_t layerMask, QueryTriggerInteracation queryTriggerInteraction) {
	return FindFirstCastHit(SphereCastAll(origin, radius, direction, maxDistance, layerMask, queryTriggerInteraction), hitInfo);
}


std::vector<RaycastHit> ou::Physics::SphereCastAll(const Vector3 &origin, float radius, const Vector3 &direction, float maxDistance, uint32_t layerMask, QueryTriggerInteracation queryTriggerInteraction) {
	SyncPendingColliders();
	return ToCastHits(PhysicsWorld::SphereCastAll(origin, radius, direction, maxDistance, layerMask, HitsTriggers(queryTriggerInteraction)), direction);
}


bool ou::Physics::CapsuleCast(const Vector3 &point1, const Vector3 &point2, float radius, const Vector3 &direction, RaycastHit *hitInfo, float maxDistance, uint32_t layerMask, QueryTriggerInteracation queryTriggerInteraction) {
	return FindFirstCastHit(CapsuleCastAll(point1, point2, radius, direction, maxDistance, layerMask, queryTriggerInteraction), hitInfo);
}


std::vector<RaycastHit> ou::Physics::CapsuleCastAll(const Vector3 &point1, const Vector3 &point2, float radius, const Vector3 &direction, float maxDistance, uint32_t layerMask, QueryTriggerInteracation queryTriggerInteraction) {
	SyncPendingColliders();
	return ToCastHits(PhysicsWorld::CapsuleCastAll(point1, point2, radius, direction, maxDistance, layerMask, HitsTriggers(queryTriggerInteraction)), direction);
}


bool ou::Physics::CheckBox(const Vector3 &center, const Vector3 &halfExtents, const Quaternion &orientation, uint32_t layerMask, QueryTriggerInteracation queryTriggerInteraction) {
	return !OverlapBox(center, halfExtents, orientation, layerMask, queryTriggerInteraction).empty();
}


bool ou::Physics::CheckSphere(const Vector3 &position, float radius, uint32_t layerMask, QueryTriggerInteracation queryTriggerInteraction) {
	return !OverlapSphere(position, radius, layerMask, queryTriggerInteraction).empty();
}


bool ou::Physics::CheckCapsule(const Vector3 &start, const Vector3 &end, float radius, uint32_t layerMask, QueryTriggerInteracation queryTriggerInteraction) {
	return !OverlapCapsule(start, end, radius, layerMask, queryTriggerInteraction).empty();
}


void ou::Physics::RegisterCollider(Collider *collider) {
	sColliders_.push_back(collider);
	sPendingColliders_.push_back(collider);
}


void ou::Physics::UnregisterCollider(Collider *collider) {
	std::erase(sColliders_, collider);
	std::erase(sPendingColliders_, collider);
}


void ou::Physics::SyncPendingColliders() {
	for (auto *collider : std::exchange(sPendingColliders_, {})) {
		collider->SyncToPhysics();
	}
}


void ou::Physics::RegisterRigidbody(Rigidbody *rigidbody) {
	sRigidbodies_.push_back(rigidbody);
}


void ou::Physics::UnregisterRigidbody(Rigidbody *rigidbody) {
	std::erase(sRigidbodies_, rigidbody);
}


void ou::Physics::DispatchContactEvents() {
	const auto events = PhysicsWorld::TakeContactEvents();
	if (events.empty()) { return; }

	std::unordered_map<PhysicsWorld::ShapeId, Collider *> colliders;
	for (auto *collider : sColliders_) {
		if (collider->shapeId_ != PhysicsWorld::kInvalidShapeId) {
			colliders.emplace(collider->shapeId_, collider);
		}
	}

	for (const auto &event : events) {
		const auto itA = colliders.find(event.shapeA);
		const auto itB = colliders.find(event.shapeB);
		if (itA == colliders.end() || itB == colliders.end()) { continue; }

		auto *a = itA->second;
		auto *b = itB->second;
		if (!a->IsActiveAndEnabled() || !b->IsActiveAndEnabled()) { continue; }

		const auto *rigidbodyA = a->GetAttachedRigidbody();
		const auto *rigidbodyB = b->GetAttachedRigidbody();
		if (rigidbodyA && rigidbodyA == rigidbodyB) { continue; }

		if (a->IsTrigger() || b->IsTrigger()) {
			if (!CanReceiveTrigger(a) && !CanReceiveTrigger(b)) { continue; }

			const auto message = SelectMessage(
				event.type, &Component::OnTriggerEnter, &Component::OnTriggerStay, &Component::OnTriggerExit);
			SendToReceivers(a, [&](Component *component) { (component->*message)(b); });
			SendToReceivers(b, [&](Component *component) { (component->*message)(a); });

		} else {
			const auto message = SelectMessage(
				event.type, &Component::OnCollisionEnter, &Component::OnCollisionStay, &Component::OnCollisionExit);
			const auto velocityA = rigidbodyA ? rigidbodyA->linearVelocityBeforeStep_ : Vector3::Zero;
			const auto velocityB = rigidbodyB ? rigidbodyB->linearVelocityBeforeStep_ : Vector3::Zero;
			const auto collisionA = MakeCollision(event, a, b, velocityB - velocityA, true);
			const auto collisionB = MakeCollision(event, b, a, velocityA - velocityB, false);
			SendToReceivers(a, [&](Component *component) { (component->*message)(collisionA); });
			SendToReceivers(b, [&](Component *component) { (component->*message)(collisionB); });
		}
	}
}


void ou::Physics::SendToReceivers(Collider *collider, const std::function<void(Component *)> &send) {
	auto *rigidbody = collider->GetAttachedRigidbody();
	GameObject *receivers[] = {collider->GetGameObject(), rigidbody ? rigidbody->GetGameObject() : nullptr};
	if (receivers[1] == receivers[0]) { receivers[1] = nullptr; }

	for (auto *receiver : receivers) {
		if (!receiver) { continue; }

		const auto count = receiver->components_.size();
		for (size_t i = 0; i < count; i++) {
			auto *comp = receiver->components_[i].get();
			if (!comp || comp->IsDestroyed()) { continue; }
			send(comp);
		}
	}
}


bool ou::Physics::HitsTriggers(QueryTriggerInteracation queryTriggerInteraction) {
	switch (queryTriggerInteraction) {
		case QueryTriggerInteracation::Ignore:
			return false;
		case QueryTriggerInteracation::Collide:
			return true;
		default:
			return sQueryHitTriggers_;
	}
}


Collider *ou::Physics::FindCollider(PhysicsWorld::ShapeId id) {
	for (auto *collider : sColliders_) {
		if (collider->shapeId_ == id) { return collider; }
	}
	return nullptr;
}


std::vector<Collider *> ou::Physics::FindColliders(const std::vector<PhysicsWorld::ShapeId> &ids) {
	std::vector<Collider *> colliders;
	for (const auto id : ids) {
		if (auto *collider = FindCollider(id)) { colliders.push_back(collider); }
	}
	return colliders;
}


std::vector<RaycastHit> ou::Physics::ToCastHits(const std::vector<PhysicsWorld::RaycastHit> &results, const Vector3 &direction) {
	std::vector<RaycastHit> hits;

	for (const auto &result : results) {
		auto *collider = FindCollider(result.shapeId);
		if (!collider) { continue; }

		if (result.distance <= 0.0f) {
			hits.push_back({Vector3::Zero, -direction.Normalized(), 0.0f, collider});
		} else {
			hits.push_back({result.point, result.normal, result.distance, collider});
		}
	}

	return hits;
}


bool ou::Physics::FindFirstCastHit(const std::vector<RaycastHit> &hits, RaycastHit *hitInfo) {
	for (const auto &hit : hits) {
		if (hit.distance <= 0.0f) { continue; }

		if (hitInfo) { *hitInfo = hit; }
		return true;
	}

	return false;
}


PhysicsWorld::CollisionFilter ou::Physics::CalculateCollisionFilter(int layer) {
	return {1u << layer, ~sIgnoreLayerMasks_[layer]};
}
