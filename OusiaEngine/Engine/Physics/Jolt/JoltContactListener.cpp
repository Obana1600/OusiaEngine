#include "JoltContactListener.h"

#include <utility>

#include <Jolt/Physics/Body/Body.h>
#include <Jolt/Physics/Collision/Shape/CompoundShape.h>

using namespace ou;


namespace {

PhysicsWorld::ShapeId FindShapeId(const JPH::Body &body, const JPH::SubShapeID &subShapeId) {
	const auto *shape = body.GetShape();
	if (shape->GetSubType() != JPH::EShapeSubType::MutableCompound) { return PhysicsWorld::kInvalidShapeId; }

	const auto *compound = static_cast<const JPH::CompoundShape *>(shape);
	JPH::SubShapeID remainder;
	return compound->GetCompoundUserData(compound->GetSubShapeIndexFromID(subShapeId, remainder));
}

} // namespace



void ou::JoltContactListener::OnContactAdded(const JPH::Body &body1, const JPH::Body &body2,
											 const JPH::ContactManifold &manifold, JPH::ContactSettings &) {
	Report(body1, body2, manifold);
}


void ou::JoltContactListener::OnContactPersisted(const JPH::Body &body1, const JPH::Body &body2,
												 const JPH::ContactManifold &manifold, JPH::ContactSettings &) {
	Report(body1, body2, manifold);
}


void ou::JoltContactListener::OnContactRemoved(const JPH::SubShapeIDPair &pair) {
	Contact contact;
	contact.key = pair;
	contact.isRemoved = true;

	std::lock_guard lock(mutex_);
	contacts_.push_back(std::move(contact));
}


void ou::JoltContactListener::TakeContacts(std::vector<Contact> *contacts) {
	std::lock_guard lock(mutex_);
	*contacts = std::exchange(contacts_, {});
}


void ou::JoltContactListener::Report(const JPH::Body &body1, const JPH::Body &body2, const JPH::ContactManifold &manifold) {
	Contact contact;
	contact.key = JPH::SubShapeIDPair(body1.GetID(), manifold.mSubShapeID1, body2.GetID(), manifold.mSubShapeID2);
	contact.shapeA = FindShapeId(body1, manifold.mSubShapeID1);
	contact.shapeB = FindShapeId(body2, manifold.mSubShapeID2);
	contact.normal = manifold.mWorldSpaceNormal;
	contact.penetrationDepth = manifold.mPenetrationDepth;
	for (JPH::uint i = 0; i < manifold.mRelativeContactPointsOn1.size(); i++) {
		contact.points.push_back(manifold.GetWorldSpaceContactPointOn1(i));
	}

	std::lock_guard lock(mutex_);
	contacts_.push_back(std::move(contact));
}
