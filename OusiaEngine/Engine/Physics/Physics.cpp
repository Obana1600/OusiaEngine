#include "Physics.h"
#include "Core/Time.h"
#include "Physics/Collider.h"
#include "Physics/Collision.h"
#include "Physics/Box3D/PhysicsWorld.h"
#include "Physics/Rigidbody.h"
#include "Scene/GameObject.h"

#include <algorithm>
#include <unordered_map>
#include <utility>

using namespace ou;


namespace {

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

Collision MakeCollision(const PhysicsWorld::ContactEvent &event, Collider *self, Collider *other, bool isShapeA) {
	const auto normal = isShapeA ? -event.normal : event.normal;

	std::vector<ContactPoint> contacts;
	for (const auto &point : event.points) {
		contacts.push_back({point, normal, event.separation, self, other});
	}
	return Collision(other, other->GetAttachedRigidbody(), std::move(contacts));
}

} // namespace



void ou::Physics::Simulate() {
	// 剛体の更新
	SyncTransform();
	PhysicsWorld::Step(Time::GetDeltaTime());
	for (auto *rigidbody : sRigidbodies_) {
		rigidbody->WriteBackPose();
	}

	DispatchContactEvents();
}


void ou::Physics::SyncTransform() {
	for (auto *rigidbody : sRigidbodies_) {
		rigidbody->SyncBodyPose();
	}
	for (auto *collider : sColliders_) {
		collider->SyncToPhysics();
	}
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


bool ou::Physics::Raycast(const Vector3 &origin, const Vector3 &direction, RaycastHit *hitInfo, float maxDistance, QueryTriggerInteracation queryTriggerInteraction) {
	const auto hits = RaycastAll(origin, direction, maxDistance, queryTriggerInteraction);
	if (hits.empty()) { return false; }

	if (hitInfo) { *hitInfo = hits.front(); }
	return true;
}


std::vector<RaycastHit> ou::Physics::RaycastAll(const Vector3 &origin, const Vector3 &direction, float maxDistance, QueryTriggerInteracation queryTriggerInteraction) {
	std::vector<RaycastHit> hits;
	for (const auto &result : PhysicsWorld::RaycastAll(origin, direction, maxDistance, HitsTriggers(queryTriggerInteraction))) {
		if (auto *collider = FindCollider(result.shapeId)) {
			hits.push_back({result.point, result.normal, result.distance, collider});
		}
	}
	return hits;
}


std::vector<Collider *> ou::Physics::OverlapBox(const Vector3 &center, const Vector3 &halfExtents, const Quaternion &orientation, QueryTriggerInteracation queryTriggerInteraction) {
	return FindColliders(PhysicsWorld::OverlapBox(center, halfExtents, orientation, HitsTriggers(queryTriggerInteraction)));
}


std::vector<Collider *> ou::Physics::OverlapSphere(const Vector3 &center, float radius, QueryTriggerInteracation queryTriggerInteraction) {
	return FindColliders(PhysicsWorld::OverlapSphere(center, radius, HitsTriggers(queryTriggerInteraction)));
}


std::vector<Collider *> ou::Physics::OverlapCapsule(const Vector3 &point0, const Vector3 &point1, float radius, QueryTriggerInteracation queryTriggerInteraction) {
	return FindColliders(PhysicsWorld::OverlapCapsule(point0, point1, radius, HitsTriggers(queryTriggerInteraction)));
}


void ou::Physics::RegisterCollider(Collider *collider) {
	sColliders_.push_back(collider);
}


void ou::Physics::UnregisterCollider(Collider *collider) {
	std::erase(sColliders_, collider);
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
			if (!rigidbodyA && !rigidbodyB) { continue; }

			const auto message = SelectMessage(
				event.type, &Component::OnTriggerEnter, &Component::OnTriggerStay, &Component::OnTriggerExit);
			SendToReceivers(a, [&](Component *component) { (component->*message)(b); });
			SendToReceivers(b, [&](Component *component) { (component->*message)(a); });

		} else {
			const auto message = SelectMessage(
				event.type, &Component::OnCollisionEnter, &Component::OnCollisionStay, &Component::OnCollisionExit);
			const auto collisionA = MakeCollision(event, a, b, true);
			const auto collisionB = MakeCollision(event, b, a, false);
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
