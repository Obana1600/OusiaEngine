#include "PhysicsWorld.h"
#include "Diagnostics/Log.h"
#include "Math/Mathf.h"

#include <Windows.h>
#include <algorithm>
#include <map>
#include <set>
#include <thread>
#include <utility>

#include <box3d/box3d.h>

using namespace ou;


namespace {

// 1 ステップを分けて処理する回数
constexpr int kSubStepCount = 4;

// 重力の既定値
constexpr b3Vec3 kDefaultGravity = {0.0f, -9.81f, 0.0f};

// 形の大きさの下限
constexpr float kMinShapeExtent = 0.001f;

// 動かした Body の周りで起こす範囲の余白
constexpr float kWakeMargin = 0.1f;

// Raycast の最大距離
constexpr float kMaxRaycastDistance = 100000.0f;

// Stay コリジョン 1つぶん
struct ActiveContact {
	b3ShapeId shapeA = b3_nullShapeId;
	b3ShapeId shapeB = b3_nullShapeId;
	b3ContactId contactId = b3_nullContactId;
};

// Stay コリジョンの組
using ContactKey = std::pair<PhysicsWorld::ShapeId, PhysicsWorld::ShapeId>;

//
struct RaycastContext {
	std::vector<PhysicsWorld::RaycastHit> hits;
	float distance = 0.0f;
	bool hitsSensors = true;
};

//
struct OverlapContext {
	std::vector<PhysicsWorld::ShapeId> shapes;
	bool hitsSensors = false;
};


// グローバルなインスタンス
b3WorldId sWorld = b3_nullWorldId;

std::map<ContactKey, ActiveContact> sActiveContacts;


void LogImpl(const char *message) {
	Log::Warning("Box3D: {}", message);
}

int AssertImpl(const char *condition, const char *fileName, int lineNumber) {
	Log::Error("Box3D assert: {} ({}:{})", condition, fileName, lineNumber);
	return IsDebuggerPresent() ? 1 : 0;
}


b3Vec3 ToB3(const Vector3 &v) {
	return {v.x, v.y, v.z};
}

b3Quat ToB3(const Quaternion &q) {
	const auto n = q.Normalized();
	return {{n.x, n.y, n.z}, n.w};
}

Vector3 ToOu(const b3Vec3 &v) {
	return Vector3(v.x, v.y, v.z);
}

Quaternion ToOu(const b3Quat &q) {
	return Quaternion(q.v.x, q.v.y, q.v.z, q.s);
}

b3BodyType ToB3(PhysicsWorld::MotionType motionType) {
	switch (motionType) {
		case PhysicsWorld::MotionType::Kinematic:
			return b3_kinematicBody;
		case PhysicsWorld::MotionType::Dynamic:
			return b3_dynamicBody;
		default:
			return b3_staticBody;
	}
}

Vector3 Abs(const Vector3 &v) {
	return Vector3(Mathf::Abs(v.x), Mathf::Abs(v.y), Mathf::Abs(v.z));
}

b3BodyId FindBody(PhysicsWorld::BodyId id) {
	const auto body = b3LoadBodyId(id);
	return b3Body_IsValid(body) ? body : b3_nullBodyId;
}

b3ShapeId FindShape(PhysicsWorld::ShapeId id) {
	const auto shape = b3LoadShapeId(id);
	return b3Shape_IsValid(shape) ? shape : b3_nullShapeId;
}

b3BoxHull MakeBoxHull(const PhysicsWorld::ShapeGeometry &geometry) {
	const auto halfExtents = Vector3::Max(Vector3::Scale(Abs(geometry.halfExtents), Abs(geometry.scale)),
										  Vector3::One * kMinShapeExtent);
	const b3Transform transform = {ToB3(geometry.localPosition), ToB3(geometry.localRotation)};
	return b3MakeTransformedBoxHull(halfExtents.x, halfExtents.y, halfExtents.z, transform);
}

b3Sphere MakeSphere(const PhysicsWorld::ShapeGeometry &geometry) {
	const auto scale = Abs(geometry.scale);
	const auto radius = Mathf::Abs(geometry.radius) * Mathf::Max(scale.x, scale.y, scale.z);
	return {ToB3(geometry.localPosition),
			Mathf::Max(radius, kMinShapeExtent)};
}

b3Capsule MakeCapsule(const PhysicsWorld::ShapeGeometry &geometry) {
	const auto scale = Abs(geometry.scale);
	const auto axis = std::clamp(geometry.direction, 0, 2);
	const auto radius = Mathf::Abs(geometry.radius) * Mathf::Max(scale[(axis + 1) % 3], scale[(axis + 2) % 3]);
	const auto height = Mathf::Abs(geometry.height) * scale[axis];
	const auto halfLength = Mathf::Max(height * 0.5f - radius, 0.0f);

	auto direction = Vector3::Zero;
	direction[axis] = 1.0f;
	const auto offset = (geometry.localRotation * direction) * halfLength;
	return {ToB3(geometry.localPosition - offset),
			ToB3(geometry.localPosition + offset),
			Mathf::Max(radius, kMinShapeExtent)};
}

void ApplyMass(b3BodyId body, float mass) {
	if (b3Body_GetType(body) != b3_dynamicBody) { return; }

	b3Body_ApplyMassFromShapes(body);
	auto massData = b3Body_GetMassData(body);
	if (massData.mass > 0.0f) {
		massData.inertia = b3MulSM(mass / massData.mass, massData.inertia);
	} else {
		massData.center = b3Vec3_zero;
		massData.inertia = b3Mat3_identity;
	}
	massData.mass = mass;
	b3Body_SetMassData(body, massData);
}

bool CollectBody(b3ShapeId id, void *context) {
	static_cast<std::vector<b3BodyId> *>(context)->push_back(b3Shape_GetBody(id));
	return true;
}

void WakeBodiesInBounds(const b3AABB &bounds) {
	std::vector<b3BodyId> bodies;
	b3World_OverlapAABB(sWorld, b3AABB_Inflate(bounds, kWakeMargin), b3DefaultQueryFilter(), CollectBody, &bodies);

	for (const auto &body : bodies) {
		b3Body_SetAwake(body, true);
	}
}


ContactKey MakeContactKey(b3ShapeId shapeA, b3ShapeId shapeB) {
	const auto a = b3StoreShapeId(shapeA);
	const auto b = b3StoreShapeId(shapeB);
	return a < b ? ContactKey(a, b) : ContactKey(b, a);
}

bool IsBodyAwake(b3ShapeId shape) {
	return b3Body_IsAwake(b3Shape_GetBody(shape));
}

PhysicsWorld::ContactEvent MakeContactEvent(PhysicsWorld::ContactEvent::Type type, const ActiveContact &contact) {
	PhysicsWorld::ContactEvent event;
	event.type = type;
	event.shapeA = b3StoreShapeId(contact.shapeA);
	event.shapeB = b3StoreShapeId(contact.shapeB);

	if (B3_IS_NULL(contact.contactId) || !b3Contact_IsValid(contact.contactId)) { return event; }

	const auto data = b3Contact_GetData(contact.contactId);
	if (data.manifoldCount == 0) { return event; }

	const auto centerA = ToOu(b3Body_GetWorldCenterOfMass(b3Shape_GetBody(data.shapeIdA)));
	event.normal = ToOu(data.manifolds[0].normal);
	event.separation = Mathf::Infinity;
	for (int i = 0; i < data.manifoldCount; i++) {
		const auto &manifold = data.manifolds[i];
		for (int j = 0; j < manifold.pointCount; j++) {
			event.points.push_back(centerA + ToOu(manifold.points[j].anchorA));
			event.separation = Mathf::Min(event.separation, manifold.points[j].separation);
		}
	}
	if (event.points.empty()) { event.separation = 0.0f; }
	return event;
}


float CollectRaycastHit(b3ShapeId shape, b3Pos point, b3Vec3 normal, float fraction, uint64_t, int, int, void *context) {
	auto *raycast = static_cast<RaycastContext *>(context);

	if (fraction <= 0.0f) { return -1.0f; }
	if (!raycast->hitsSensors && b3Shape_IsSensor(shape)) { return -1.0f; }

	raycast->hits.push_back({b3StoreShapeId(shape),
							 ToOu(point),
							 ToOu(normal),
							 fraction * raycast->distance});

	return 1.0f;
}

bool CollectOverlap(b3ShapeId shape, void *context) {
	auto *overlap = static_cast<OverlapContext *>(context);
	if (overlap->hitsSensors || !b3Shape_IsSensor(shape)) {
		overlap->shapes.push_back(b3StoreShapeId(shape));
	}
	return true;
}

std::vector<PhysicsWorld::ShapeId> OverlapProxy(const Vector3 &origin, const b3Vec3 *points, int count, float radius, bool hitsSensors) {
	if (B3_IS_NULL(sWorld)) { return {}; }

	const b3ShapeProxy proxy = {points, count, radius};
	OverlapContext overlap;
	overlap.hitsSensors = hitsSensors;
	b3World_OverlapShape(sWorld, ToB3(origin), &proxy, b3DefaultQueryFilter(), CollectOverlap, &overlap);
	return overlap.shapes;
}

} // namespace



void ou::PhysicsWorld::Initialize() {
	if (B3_IS_NON_NULL(sWorld)) { return; }

	b3SetLogFcn(LogImpl);
	b3SetAssertFcn(AssertImpl);

	auto def = b3DefaultWorldDef();
	def.gravity = kDefaultGravity;
	def.workerCount = std::clamp(std::thread::hardware_concurrency(), 1u, static_cast<uint32_t>(B3_MAX_WORKERS));
	sWorld = b3CreateWorld(&def);

	const auto version = b3GetVersion();
	Log::Info("Box3D {}.{}.{} を初期化しました (並列数 {})",
			  version.major, version.minor, version.revision, b3World_GetWorkerCount(sWorld));
}


void ou::PhysicsWorld::Finalize() {
	if (B3_IS_NULL(sWorld)) { return; }

	sActiveContacts.clear();

	b3DestroyWorld(sWorld);
	sWorld = b3_nullWorldId;
}


bool ou::PhysicsWorld::IsInitialized() {
	return B3_IS_NON_NULL(sWorld);
}


void ou::PhysicsWorld::Step(float deltaTime) {
	if (B3_IS_NULL(sWorld) || deltaTime <= 0.0f) { return; }
	b3World_Step(sWorld, deltaTime, kSubStepCount);
}


PhysicsWorld::BodyId ou::PhysicsWorld::CreateBody(const Vector3 &position, const Quaternion &rotation, MotionType motionType) {
	if (B3_IS_NULL(sWorld)) { return kInvalidBodyId; }

	auto def = b3DefaultBodyDef();
	def.type = ToB3(motionType);
	def.position = ToB3(position);
	def.rotation = ToB3(rotation);

	const auto body = b3CreateBody(sWorld, &def);
	if (B3_IS_NULL(body)) {
		Log::Error("Body の作成に失敗しました");
		return kInvalidBodyId;
	}

	ApplyMass(body, 1.0f);
	return b3StoreBodyId(body);
}


void ou::PhysicsWorld::DestroyBody(BodyId id) {
	const auto body = FindBody(id);
	if (B3_IS_NULL(body)) { return; }

	b3DestroyBody(body);
}


void ou::PhysicsWorld::SetBodyPose(BodyId id, const Vector3 &position, const Quaternion &rotation) {
	const auto body = FindBody(id);
	if (B3_IS_NULL(body)) { return; }

	const auto before = b3Body_ComputeAABB(body);
	b3Body_SetTransform(body, ToB3(position), ToB3(rotation));
	b3Body_SetAwake(body, true);

	const auto after = b3Body_ComputeAABB(body);
	WakeBodiesInBounds(b3AABB_Union(before, after));
}


void ou::PhysicsWorld::GetBodyPose(BodyId id, Vector3 *position, Quaternion *rotation) {
	const auto body = FindBody(id);
	if (B3_IS_NULL(body)) { return; }

	if (position) { *position = ToOu(b3Body_GetPosition(body)); }
	if (rotation) { *rotation = ToOu(b3Body_GetRotation(body)); }
}


void ou::PhysicsWorld::SetMass(BodyId id, float mass) {
	const auto body = FindBody(id);
	if (B3_IS_NULL(body)) { return; }

	ApplyMass(body, mass);
}


void ou::PhysicsWorld::SetUseGravity(BodyId id, bool useGravity) {
	const auto body = FindBody(id);
	if (B3_IS_NULL(body)) { return; }

	b3Body_SetGravityScale(body, useGravity ? 1.0f : 0.0f);
}


void ou::PhysicsWorld::SetMotionType(BodyId id, MotionType motionType) {
	const auto body = FindBody(id);
	if (B3_IS_NULL(body)) { return; }

	b3Body_SetType(body, ToB3(motionType));
}


PhysicsWorld::ShapeId ou::PhysicsWorld::CreateShape(BodyId bodyId, const ShapeGeometry &geometry, bool isSensor) {
	const auto body = FindBody(bodyId);
	if (B3_IS_NULL(body)) { return kInvalidShapeId; }

	auto def = b3DefaultShapeDef();
	def.isSensor = isSensor;
	def.density = isSensor ? 0.0f : 1.0f;
	def.updateBodyMass = false;
	def.enableContactEvents = true;
	def.enableSensorEvents = true;

	auto shape = b3_nullShapeId;
	if (geometry.type == ShapeGeometry::Type::Box) {
		const auto hull = MakeBoxHull(geometry);
		shape = b3CreateHullShape(body, &def, &hull.base);
	} else if (geometry.type == ShapeGeometry::Type::Sphere) {
		const auto sphere = MakeSphere(geometry);
		shape = b3CreateSphereShape(body, &def, &sphere);
	} else if (geometry.type == ShapeGeometry::Type::Capsule) {
		const auto capsule = MakeCapsule(geometry);
		shape = b3CreateCapsuleShape(body, &def, &capsule);
	}

	ApplyMass(body, b3Body_GetMass(body));
	return b3StoreShapeId(shape);
}


void ou::PhysicsWorld::DestroyShape(ShapeId id) {
	const auto shape = FindShape(id);
	if (B3_IS_NULL(shape)) { return; }

	const auto body = b3Shape_GetBody(shape);
	b3DestroyShape(shape, false);
	ApplyMass(body, b3Body_GetMass(body));
}


void ou::PhysicsWorld::SetShapeGeometry(ShapeId id, const ShapeGeometry &geometry) {
	const auto shape = FindShape(id);
	if (B3_IS_NULL(shape)) { return; }

	if (geometry.type == ShapeGeometry::Type::Box) {
		const auto hull = MakeBoxHull(geometry);
		b3Shape_SetHull(shape, &hull.base);
	} else if (geometry.type == ShapeGeometry::Type::Sphere) {
		const auto sphere = MakeSphere(geometry);
		b3Shape_SetSphere(shape, &sphere);
	} else if (geometry.type == ShapeGeometry::Type::Capsule) {
		const auto capsule = MakeCapsule(geometry);
		b3Shape_SetCapsule(shape, &capsule);
	}

	const auto body = b3Shape_GetBody(shape);
	ApplyMass(body, b3Body_GetMass(body));
}


std::vector<PhysicsWorld::ContactEvent> ou::PhysicsWorld::TakeContactEvents() {
	if (B3_IS_NULL(sWorld)) { return {}; }

	const auto contactEvents = b3World_GetContactEvents(sWorld);
	const auto sensorEvents = b3World_GetSensorEvents(sWorld);

	// Begin を集める
	std::set<ContactKey> begun;
	for (int i = 0; i < contactEvents.beginCount; i++) {
		begun.insert(MakeContactKey(contactEvents.beginEvents[i].shapeIdA,
									contactEvents.beginEvents[i].shapeIdB));
	}
	for (int i = 0; i < sensorEvents.beginCount; i++) {
		begun.insert(MakeContactKey(sensorEvents.beginEvents[i].sensorShapeId,
									sensorEvents.beginEvents[i].visitorShapeId));
	}

	std::vector<ContactEvent> events;

	// 先に接触が終わった組を外す
	const auto endContact = [&](b3ShapeId shapeA, b3ShapeId shapeB) {
		const auto key = MakeContactKey(shapeA, shapeB);
		if (begun.contains(key)) { return; }

		const auto it = sActiveContacts.find(key);
		if (it == sActiveContacts.end()) { return; }

		if (b3Shape_IsValid(shapeA) && b3Shape_IsValid(shapeB)) {
			events.push_back(MakeContactEvent(ContactEvent::Type::Exit, it->second));
		}
		sActiveContacts.erase(it);
	};

	// Exit
	for (int i = 0; i < contactEvents.endCount; i++) {
		endContact(contactEvents.endEvents[i].shapeIdA,
				   contactEvents.endEvents[i].shapeIdB);
	}
	for (int i = 0; i < sensorEvents.endCount; i++) {
		endContact(sensorEvents.endEvents[i].sensorShapeId,
				   sensorEvents.endEvents[i].visitorShapeId);
	}

	std::set<ContactKey> entered;

	// 新しく接触したペアのみ集める
	const auto beginContact = [&](const ActiveContact &contact) {
		const auto key = MakeContactKey(contact.shapeA, contact.shapeB);
		const auto [it, isNew] = sActiveContacts.insert_or_assign(key, contact);
		if (!isNew) { return; }

		entered.insert(key);
		events.push_back(MakeContactEvent(ContactEvent::Type::Enter, it->second));
	};

	// Enter
	for (int i = 0; i < contactEvents.beginCount; i++) {
		const auto &begin = contactEvents.beginEvents[i];
		beginContact({begin.shapeIdA, begin.shapeIdB, begin.contactId});
	}
	for (int i = 0; i < sensorEvents.beginCount; i++) {
		const auto &begin = sensorEvents.beginEvents[i];
		beginContact({begin.sensorShapeId, begin.visitorShapeId, b3_nullContactId});
	}

	// Stay
	for (auto it = sActiveContacts.begin(); it != sActiveContacts.end();) {
		const auto &contact = it->second;
		if (!b3Shape_IsValid(contact.shapeA) || !b3Shape_IsValid(contact.shapeB)) {
			it = sActiveContacts.erase(it);
			continue;
		}

		if (!entered.contains(it->first) && (IsBodyAwake(contact.shapeA) || IsBodyAwake(contact.shapeB))) {
			events.push_back(MakeContactEvent(ContactEvent::Type::Stay, contact));
		}
		it++;
	}

	return events;
}


void ou::PhysicsWorld::SetAllowedDOFs(BodyId id, uint8_t dofs) {
	const auto body = FindBody(id);
	if (B3_IS_NULL(body)) { return; }

	b3MotionLocks locks{};
	locks.linearX = (dofs & DOF::TranslationX) == 0;
	locks.linearY = (dofs & DOF::TranslationY) == 0;
	locks.linearZ = (dofs & DOF::TranslationZ) == 0;
	locks.angularX = (dofs & DOF::RotationX) == 0;
	locks.angularY = (dofs & DOF::RotationY) == 0;
	locks.angularZ = (dofs & DOF::RotationZ) == 0;

	const auto mass = b3Body_GetMass(body);
	b3Body_SetMotionLocks(body, locks);
	ApplyMass(body, mass);
}


void ou::PhysicsWorld::SetLinearDamping(BodyId id, float damping) {
	const auto body = FindBody(id);
	if (B3_IS_NULL(body)) { return; }

	b3Body_SetLinearDamping(body, Mathf::Max(damping, 0.0f));
}


void ou::PhysicsWorld::SetAngularDamping(BodyId id, float damping) {
	const auto body = FindBody(id);
	if (B3_IS_NULL(body)) { return; }

	b3Body_SetAngularDamping(body, Mathf::Max(damping, 0.0f));
}


void ou::PhysicsWorld::SetLinearVelocity(BodyId id, const Vector3 &velocity) {
	const auto body = FindBody(id);
	if (B3_IS_NULL(body)) { return; }

	b3Body_SetLinearVelocity(body, ToB3(velocity));
}


Vector3 ou::PhysicsWorld::GetLinearVelocity(BodyId id) {
	const auto body = FindBody(id);
	if (B3_IS_NULL(body)) { return Vector3::Zero; }

	return ToOu(b3Body_GetLinearVelocity(body));
}


void ou::PhysicsWorld::SetAngularVelocity(BodyId id, const Vector3 &velocity) {
	const auto body = FindBody(id);
	if (B3_IS_NULL(body)) { return; }

	b3Body_SetAngularVelocity(body, ToB3(velocity));
}


Vector3 ou::PhysicsWorld::GetAngularVelocity(BodyId id) {
	const auto body = FindBody(id);
	if (B3_IS_NULL(body)) { return Vector3::Zero; }

	return ToOu(b3Body_GetAngularVelocity(body));
}


void ou::PhysicsWorld::AddForce(BodyId id, const Vector3 &force) {
	const auto body = FindBody(id);
	if (B3_IS_NULL(body)) { return; }

	b3Body_ApplyForceToCenter(body, ToB3(force), true);
}


void ou::PhysicsWorld::AddTorque(BodyId id, const Vector3 &torque) {
	const auto body = FindBody(id);
	if (B3_IS_NULL(body)) { return; }

	b3Body_ApplyTorque(body, ToB3(torque), true);
}


void ou::PhysicsWorld::AddImpulse(BodyId id, const Vector3 &impulse) {
	const auto body = FindBody(id);
	if (B3_IS_NULL(body)) { return; }

	b3Body_ApplyLinearImpulseToCenter(body, ToB3(impulse), true);
}


void ou::PhysicsWorld::AddAngularImpulse(BodyId id, const Vector3 &impulse) {
	const auto body = FindBody(id);
	if (B3_IS_NULL(body)) { return; }

	b3Body_ApplyAngularImpulse(body, ToB3(impulse), true);
}


void ou::PhysicsWorld::MoveKinematic(BodyId id, const Vector3 &position, const Quaternion &rotation, float deltaTime) {
	const auto body = FindBody(id);
	if (B3_IS_NULL(body)) { return; }

	b3Body_SetAwake(body, true);

	const b3WorldTransform target = {ToB3(position), ToB3(rotation)};
	b3Body_SetTargetTransform(body, target, deltaTime, true);
}


void ou::PhysicsWorld::SetGravity(const Vector3 &gravity) {
	if (B3_IS_NULL(sWorld)) { return; }

	b3World_SetGravity(sWorld, ToB3(gravity));
}


Vector3 ou::PhysicsWorld::GetGravity() {
	if (B3_IS_NULL(sWorld)) { return Vector3::Zero; }

	return ToOu(b3World_GetGravity(sWorld));
}


std::vector<PhysicsWorld::RaycastHit> ou::PhysicsWorld::RaycastAll(const Vector3 &origin, const Vector3 &direction, float maxDistance, bool hitsSensors) {
	const auto length = direction.Length();
	if (B3_IS_NULL(sWorld) || length <= 0.0f || maxDistance <= 0.0f) { return {}; }

	RaycastContext raycast;
	raycast.distance = Mathf::Min(maxDistance, kMaxRaycastDistance);
	raycast.hitsSensors = hitsSensors;
	b3World_CastRay(sWorld, ToB3(origin), ToB3(direction / length * raycast.distance),
					b3DefaultQueryFilter(), CollectRaycastHit, &raycast);

	std::sort(raycast.hits.begin(), raycast.hits.end(),
			  [](const RaycastHit &a, const RaycastHit &b) { return a.distance < b.distance; });
	return raycast.hits;
}


std::vector<PhysicsWorld::ShapeId> ou::PhysicsWorld::OverlapBox(const Vector3 &center, const Vector3 &halfExtents, const Quaternion &rotation, bool hitsSensors) {
	const auto extents = Abs(halfExtents);

	b3Vec3 corners[8];
	for (int i = 0; i < 8; i++) {
		const Vector3 corner = {(i & 1) ? extents.x : -extents.x,
								(i & 2) ? extents.y : -extents.y,
								(i & 4) ? extents.z : -extents.z};
		corners[i] = ToB3(rotation * corner);
	}

	return OverlapProxy(center, corners, 8, 0.0f, hitsSensors);
}


std::vector<PhysicsWorld::ShapeId> ou::PhysicsWorld::OverlapSphere(const Vector3 &center, float radius, bool hitsSensors) {
	const b3Vec3 point = b3Vec3_zero;
	return OverlapProxy(center, &point, 1, Mathf::Abs(radius), hitsSensors);
}


std::vector<PhysicsWorld::ShapeId> ou::PhysicsWorld::OverlapCapsule(const Vector3 &point0, const Vector3 &point1, float radius, bool hitsSensors) {
	const auto center = (point0 + point1) * 0.5f;
	const b3Vec3 points[2] = {ToB3(point0 - center),
							  ToB3(point1 - center)};
	return OverlapProxy(center, points, 2, Mathf::Abs(radius), hitsSensors);
}
