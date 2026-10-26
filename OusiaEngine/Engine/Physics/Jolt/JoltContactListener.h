#pragma once

#include "Physics/Jolt/PhysicsWorld.h"

#include <mutex>
#include <vector>

// clang-format off
#include <Jolt/Jolt.h> // Jolt.h を先に置く
#include <Jolt/Physics/Collision/ContactListener.h>
// clang-format on


namespace ou {

class JoltContactListener final: public JPH::ContactListener {
public: // インナークラス

	struct Contact {
		JPH::SubShapeIDPair key;
		bool isRemoved = false;
		PhysicsWorld::ShapeId shapeA = PhysicsWorld::kInvalidShapeId;
		PhysicsWorld::ShapeId shapeB = PhysicsWorld::kInvalidShapeId;
		JPH::Vec3 normal = JPH::Vec3::sZero();
		float penetrationDepth = 0.0f;
		std::vector<JPH::RVec3> points;
	};


public: // 静的メンバ関数

	static PhysicsWorld::ShapeId FindShapeId(const JPH::Body &body, const JPH::SubShapeID &subShapeId);


public: // メンバ関数

	void OnContactAdded(const JPH::Body &body1, const JPH::Body &body2,
						const JPH::ContactManifold &manifold, JPH::ContactSettings &settings) override;
	void OnContactPersisted(const JPH::Body &body1, const JPH::Body &body2,
							const JPH::ContactManifold &manifold, JPH::ContactSettings &settings) override;
	void OnContactRemoved(const JPH::SubShapeIDPair &pair) override;

	void TakeContacts(std::vector<Contact> *contacts);


private: // メンバ変数

	std::mutex mutex_;
	std::vector<Contact> contacts_;


private: // メンバ関数

	void Report(const JPH::Body &body1, const JPH::Body &body2, const JPH::ContactManifold &manifold);
};

} // namespace ou
