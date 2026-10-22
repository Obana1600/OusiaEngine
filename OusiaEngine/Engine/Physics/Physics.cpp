#include "Physics.h"
#include "Math/Mathf.h"
#include "Physics/BoxCollider.h"
#include "Physics/Collider.h"
#include "Physics/SphereCollider.h"
#include "Scene/GameObject.h"

#include <algorithm>

using namespace ou;


namespace {

constexpr float kParallelEpsilon = 1e-6f;

Physics::ContactPair MakeContactPair(Collider *a, Collider *b) {
	return a->GetInstanceId() < b->GetInstanceId() ? Physics::ContactPair{a, b}
												   : Physics::ContactPair{b, a};
}

} // namespace



void ou::Physics::Simulate() {
	// 今フレームの接触を集める
	std::vector<ContactPair> currentContacts;
	for (size_t i = 0; i < sColliders_.size(); i++) {
		for (size_t j = i + 1; j < sColliders_.size(); j++) {
			auto *a = sColliders_[i];
			auto *b = sColliders_[j];

			if (a->GetGameObject() == b->GetGameObject()) { continue; }
			if (!a->IsTrigger() && !b->IsTrigger()) { continue; }
			if (!a->GetBounds().Intersects(b->GetBounds())) { continue; }
			if (!Intersects(a, b)) { continue; }

			currentContacts.push_back(MakeContactPair(a, b));
		}
	}

	// 前フレームと比べてイベントを作る
	std::vector<std::pair<ContactPair, Message>> events;
	for (const auto &pair : currentContacts) {
		const auto wasTouching = std::ranges::find(sContacts_, pair) != sContacts_.end();
		events.emplace_back(pair, wasTouching ? &Component::OnTriggerStay : &Component::OnTriggerEnter);
	}
	for (const auto &pair : sContacts_) {
		if (std::ranges::find(currentContacts, pair) == currentContacts.end()) {
			events.emplace_back(pair, &Component::OnTriggerExit);
		}
	}

	sContacts_ = std::move(currentContacts);

	// まとめて送る
	for (const auto &[pair, message] : events) {
		if (!pair.a->IsActiveAndEnabled() || !pair.b->IsActiveAndEnabled()) { continue; }
		SendTriggerMessage(pair, message);
	}
}


void ou::Physics::RegisterCollider(Collider *collider) {
	sColliders_.push_back(collider);
}


void ou::Physics::UnregisterCollider(Collider *collider) {
	std::erase(sColliders_, collider);
	std::erase_if(sContacts_, [collider](const auto &pair) {
		return pair.a == collider || pair.b == collider;
	});
}


void ou::Physics::SendTriggerMessage(const ContactPair &pair, Message message) {
	auto sendToComponents = [message](GameObject *gameObject, Collider *other) {
		const auto count = gameObject->components_.size();
		for (size_t i = 0; i < count; i++) {
			auto *component = gameObject->components_[i].get();
			if (component->IsDestroyed()) { continue; }
			(component->*message)(other);
		}
	};

	sendToComponents(pair.a->GetGameObject(), pair.b);
	sendToComponents(pair.b->GetGameObject(), pair.a);
}


bool ou::Physics::Intersects(const Collider *a, const Collider *b) {
	const auto aShape = a->GetShape();
	const auto bShape = b->GetShape();

	if (aShape == ColliderShape::Sphere && bShape == ColliderShape::Sphere) {
		return IntersectsSphereSphere(static_cast<const SphereCollider *>(a),
									  static_cast<const SphereCollider *>(b));
	}

	if (aShape == ColliderShape::Sphere) {
		return IntersectsSphere(static_cast<const SphereCollider *>(a), b);
	}
	if (bShape == ColliderShape::Sphere) {
		return IntersectsSphere(static_cast<const SphereCollider *>(b), a);
	}

	if (aShape == ColliderShape::Box && bShape == ColliderShape::Box) {
		return IntersectsBoxBox(static_cast<const BoxCollider *>(a),
									  static_cast<const BoxCollider *>(b));
	}

	return false;
}


bool ou::Physics::IntersectsBoxBox(const BoxCollider *a, const BoxCollider *b) {
	const auto aBox = a->CalculateWorldBox();
	const auto bBox = b->CalculateWorldBox();

	float r[3][3];
	float absR[3][3];
	for (int i = 0; i < 3; i++) {
		for (int j = 0; j < 3; j++) {
			r[i][j] = Vector3::Dot(aBox.axes[i], bBox.axes[j]);
			absR[i][j] = Mathf::Abs(r[i][j]) + kParallelEpsilon;
		}
	}

	const auto offset = bBox.center - aBox.center;
	const float t[3] = {
		Vector3::Dot(offset, aBox.axes[0]),
		Vector3::Dot(offset, aBox.axes[1]),
		Vector3::Dot(offset, aBox.axes[2])};

	const auto &ea = aBox.halfExtents;
	const auto &eb = bBox.halfExtents;

	for (int i = 0; i < 3; i++) {
		const auto radiusA = ea[i];
		const auto radiusB = eb[0] * absR[i][0] + eb[1] * absR[i][1] + eb[2] * absR[i][2];
		if (Mathf::Abs(t[i]) > radiusA + radiusB) {
			return false;
		}
	}

	for (int j = 0; j < 3; j++) {
		const auto radiusA = ea[0] * absR[0][j] + ea[1] * absR[1][j] + ea[2] * absR[2][j];
		const auto radiusB = eb[j];
		const auto distance = t[0] * r[0][j] + t[1] * r[1][j] + t[2] * r[2][j];
		if (Mathf::Abs(distance) > radiusA + radiusB) {
			return false;
		}
	}

	for (int i = 0; i < 3; i++) {
		const auto i1 = (i + 1) % 3;
		const auto i2 = (i + 2) % 3;
		for (int j = 0; j < 3; j++) {
			const auto j1 = (j + 1) % 3;
			const auto j2 = (j + 2) % 3;

			const auto radiusA = ea[i1] * absR[i2][j] + ea[i2] * absR[i1][j];
			const auto radiusB = eb[j1] * absR[i][j2] + eb[j2] * absR[i][j1];
			const auto distance = t[i2] * r[i1][j] - t[i1] * r[i2][j];
			if (Mathf::Abs(distance) > radiusA + radiusB) {
				return false;
			}
		}
	}

	return true;
}


bool ou::Physics::IntersectsSphereSphere(const SphereCollider *a, const SphereCollider *b) {
	const auto radiusSum = a->CalculateWorldRadius() + b->CalculateWorldRadius();
	const auto offset = b->CalculateWorldCenter() - a->CalculateWorldCenter();
	return offset.LengthSquared() <= radiusSum * radiusSum;
}


bool ou::Physics::IntersectsSphere(const SphereCollider *a, const Collider *b) {
	const auto center = a->CalculateWorldCenter();
	const auto radius = a->CalculateWorldRadius();

	const auto closest = b->ClosestPoint(center);
	const auto offset = closest - center;
	return offset.LengthSquared() <= radius * radius;
}
