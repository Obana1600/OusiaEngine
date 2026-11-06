#include "PhysicsWorld.h"
#include "Diagnostics/Log.h"
#include "Math/Mathf.h"

#include <Windows.h>
#include <algorithm>
#include <cfloat>
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

// Stay コリジョン 1つぶん
struct ActiveContact {
	b3ShapeId shapeA = b3_nullShapeId;
	b3ShapeId shapeB = b3_nullShapeId;
	b3ContactId contactId = b3_nullContactId;
};

// Stay コリジョンの組
using ContactKey = std::pair<PhysicsWorld::ShapeId, PhysicsWorld::ShapeId>;


// グローバルなインスタンス
b3WorldId sWorld = b3_nullWorldId;
std::map<ContactKey, ActiveContact> sActiveContacts;
std::map<PhysicsWorld::BodyId, b3Vec3> sCenterOfMassOverrides;


// Impl ===========================================================================================

void LogImpl(const char *message) {
	Log::Warning("Box3D: {}", message);
}

int AssertImpl(const char *condition, const char *fileName, int lineNumber) {
	Log::Error("Box3D assert: {} ({}:{})", condition, fileName, lineNumber);
	return IsDebuggerPresent() ? 1 : 0;
}

// ================================================================================================


// Convert ========================================================================================

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

// ================================================================================================


// Shape ==========================================================================================

// 形の大きさの下限
constexpr float kMinShapeExtent = 0.001f;


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


// SurfaceMaterial --------------------------------------------------------------------------------

// userMaterialId の中で、restitutionCombine を置く位置
constexpr int kRestitutionCombineShift = 8;


b3SurfaceMaterial ToB3(const PhysicsWorld::SurfaceMaterial &material) {
	auto result = b3DefaultSurfaceMaterial();
	result.friction = material.friction;
	result.restitution = material.restitution;
	result.userMaterialId = static_cast<uint64_t>(material.frictionCombine) |
							(static_cast<uint64_t>(material.restitutionCombine) << kRestitutionCombineShift);
	return result;
}

PhysicsWorld::CombineMode ToCombine(uint64_t bits) {
	return static_cast<PhysicsWorld::CombineMode>(bits & 0xFF);
}

float Combine(float a, float b, PhysicsWorld::CombineMode combine) {
	switch (combine) {
		default:
		case PhysicsWorld::CombineMode::Average:
			return (a + b) * 0.5f;
		case PhysicsWorld::CombineMode::Minimum:
			return Mathf::Min(a, b);
		case PhysicsWorld::CombineMode::Multiply:
			return a * b;
		case PhysicsWorld::CombineMode::Maximum:
			return Mathf::Max(a, b);
	}
}

float MixFriction(float frictionA, uint64_t userMaterialIdA, float frictionB, uint64_t userMaterialIdB) {
	const auto combine = std::max(ToCombine(userMaterialIdA), ToCombine(userMaterialIdB));
	return Combine(frictionA, frictionB, combine);
}

float MixRestitution(float restitutionA, uint64_t userMaterialIdA, float restitutionB, uint64_t userMaterialIdB) {
	const auto combine = std::max(ToCombine(userMaterialIdA >> kRestitutionCombineShift),
								  ToCombine(userMaterialIdB >> kRestitutionCombineShift));
	return Combine(restitutionA, restitutionB, combine);
}

// ------------------------------------------------------------------------------------------------


// Filter -----------------------------------------------------------------------------------------

// フィルター
constexpr uint64_t kQueryCategoryBits = 1ull << 32;

b3Filter ToB3(const PhysicsWorld::CollisionFilter &filter) {
	auto result = b3DefaultFilter();
	result.categoryBits = filter.categoryBits;
	result.maskBits = filter.maskBits | kQueryCategoryBits;
	return result;
}

b3QueryFilter MakeQueryFilter(uint32_t layerMask) {
	auto filter = b3DefaultQueryFilter();
	filter.categoryBits = kQueryCategoryBits;
	filter.maskBits = layerMask;
	return filter;
}

// ------------------------------------------------------------------------------------------------

// ================================================================================================


// Body ===========================================================================================

// 動かした Body の周りで起こす範囲の余白
constexpr float kWakeMargin = 0.1f;


b3BodyId FindBody(PhysicsWorld::BodyId id) {
	const auto body = b3LoadBodyId(id);
	return b3Body_IsValid(body) ? body : b3_nullBodyId;
}

void ApplyMass(b3BodyId body, float mass) {
	if (b3Body_GetType(body) != b3_dynamicBody) { return; }

	const auto oldCenter = b3Body_GetWorldCenterOfMass(body);
	const auto linearVelocity = b3Body_GetLinearVelocity(body);
	const auto angularVelocity = b3Body_GetAngularVelocity(body);

	b3Body_ApplyMassFromShapes(body);
	auto massData = b3Body_GetMassData(body);
	if (massData.mass > 0.0f) {
		massData.inertia = b3MulSM(mass / massData.mass, massData.inertia);
	} else {
		massData.center = b3Vec3_zero;
		massData.inertia = b3Mat3_identity;
	}
	massData.mass = mass;

	const auto found = sCenterOfMassOverrides.find(b3StoreBodyId(body));
	if (found != sCenterOfMassOverrides.end()) {
		massData.center = found->second;
	}
	b3Body_SetMassData(body, massData);

	b3Body_SetTransform(body, b3Body_GetPosition(body), b3Body_GetRotation(body));

	const auto centerShift = b3SubPos(b3Body_GetWorldCenterOfMass(body), oldCenter);
	b3Body_SetLinearVelocity(body, b3Add(linearVelocity, b3Cross(angularVelocity, centerShift)));
}

bool CollectBody(b3ShapeId id, void *context) {
	static_cast<std::vector<b3BodyId> *>(context)->push_back(b3Shape_GetBody(id));
	return true;
}

void WakeBodiesInBounds(const b3AABB &bounds) {
	std::vector<b3BodyId> bodies;
	b3World_OverlapAABB(sWorld, b3AABB_Inflate(bounds, kWakeMargin),
						MakeQueryFilter(0xFFFFFFFF), CollectBody, &bodies);

	for (const auto &body : bodies) {
		b3Body_SetAwake(body, true);
	}
}

// ================================================================================================


// Collision ======================================================================================

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
		auto normalImpulse = 0.0f; 
		for (int j = 0; j < manifold.pointCount; j++) {
			event.points.push_back(centerA + ToOu(manifold.points[j].anchorA));
			event.separation = Mathf::Min(event.separation, manifold.points[j].separation);
			normalImpulse += manifold.points[j].totalNormalImpulse;
		}

		event.impulse += ToOu(manifold.normal) * (normalImpulse * 0.5f) +
						 ToOu(manifold.frictionImpulse) * static_cast<float>(kSubStepCount);
	}
	if (event.points.empty()) { event.separation = 0.0f; }
	return event;
}

// ================================================================================================


// Raycast ========================================================================================

// Raycast の最大距離
constexpr float kMaxRaycastDistance = 100000.0f;

// レイキャストの情報
struct RaycastContext {
	std::vector<PhysicsWorld::RaycastHit> hits;
	float distance = 0.0f;
	bool hitsSensors = true;
};

// 形状の情報
struct OverlapContext {
	std::vector<PhysicsWorld::ShapeId> shapes;
	bool hitsSensors = false;
};


float CollectRaycastHit(
	b3ShapeId shape, b3Pos point, b3Vec3 normal, float fraction, uint64_t, int, int, void *context) {
	auto *raycast = static_cast<RaycastContext *>(context);

	if (fraction <= 0.0f) { return -1.0f; }
	if (!raycast->hitsSensors && b3Shape_IsSensor(shape)) { return -1.0f; }

	raycast->hits.push_back({b3StoreShapeId(shape), ToOu(point), ToOu(normal), fraction * raycast->distance});
	return 1.0f;
}

bool CollectOverlap(b3ShapeId shape, void *context) {
	auto *overlap = static_cast<OverlapContext *>(context);
	if (overlap->hitsSensors || !b3Shape_IsSensor(shape)) {
		overlap->shapes.push_back(b3StoreShapeId(shape));
	}
	return true;
}

std::vector<PhysicsWorld::ShapeId> OverlapProxy(
	const Vector3 &origin, const b3Vec3 *points, int count, float radius, uint32_t layerMask, bool hitsSensors) {
	if (B3_IS_NULL(sWorld)) { return {}; }

	const b3ShapeProxy proxy = {points, count, radius};
	OverlapContext overlap;
	overlap.hitsSensors = hitsSensors;
	b3World_OverlapShape(sWorld, ToB3(origin), &proxy, MakeQueryFilter(layerMask), CollectOverlap, &overlap);
	return overlap.shapes;
}

float CollectShapeCastHit(
	b3ShapeId shape, b3Pos point, b3Vec3 normal, float fraction, uint64_t, int, int, void *context) {
	auto *cast = static_cast<RaycastContext *>(context);
	if (!cast->hitsSensors && b3Shape_IsSensor(shape)) { return -1.0f; }

	cast->hits.push_back({b3StoreShapeId(shape), ToOu(point), ToOu(normal), fraction * cast->distance});
	return 1.0f;
}

std::vector<PhysicsWorld::RaycastHit> CastProxy(
	const Vector3 &origin, const b3Vec3 *points, int count, float radius, const Vector3 &direction, float maxDistance, uint32_t layerMask, bool hitsSensors) {
	const auto length = direction.Length();
	if (B3_IS_NULL(sWorld) || length <= 0.0f || maxDistance <= 0.0f) { return {}; }

	const b3ShapeProxy proxy = {points, count, radius};
	RaycastContext cast;
	cast.distance = Mathf::Min(maxDistance, kMaxRaycastDistance);
	cast.hitsSensors = hitsSensors;
	b3World_CastShape(sWorld, ToB3(origin), &proxy, ToB3(direction / length * cast.distance),
					  MakeQueryFilter(layerMask), CollectShapeCastHit, &cast);

	std::sort(cast.hits.begin(), cast.hits.end(),
			  [](const PhysicsWorld::RaycastHit &a, const PhysicsWorld::RaycastHit &b) { return a.distance < b.distance; });
	return cast.hits;
}

void MakeBoxCorners(const Vector3 &halfExtents, const Quaternion &rotation, b3Vec3 corners[8]) {
	const auto extents = Abs(halfExtents);
	for (int i = 0; i < 8; i++) {
		const auto corner = Vector3((i & 1) ? extents.x : -extents.x,
									(i & 2) ? extents.y : -extents.y,
									(i & 4) ? extents.z : -extents.z);
		corners[i] = ToB3(rotation * corner);
	}
}

// ================================================================================================


// Character ======================================================================================

// 面と衝突判定を繰り返す回数
constexpr int kMoverIterations = 5;

// 最小移動量
constexpr float kMoverTolerance = 0.0001f;

// カプセルの移動時に集める面
struct MoverContext {
	b3Pos origin;
	b3ShapeId ignoreShape = b3_nullShapeId;
	float minGroundNormalY = 0.0f;
	bool flattenGround = false;
	std::vector<b3CollisionPlane> planes;
	std::vector<PhysicsWorld::CapsuleMoveHit> hits;
};


bool IsMoverIgnored(b3ShapeId shape, const MoverContext &mover) {
	return B3_ID_EQUALS(shape, mover.ignoreShape) || b3Shape_IsSensor(shape);
}

bool CollectMoverPlanes(b3ShapeId shape, const b3PlaneResult *results, int count, void *context) {
	auto *mover = static_cast<MoverContext *>(context);
	if (IsMoverIgnored(shape, *mover)) { return true; }

	for (int i = 0; i < count; i++) {
		auto plane = results[i].plane;

		if (plane.normal.y > 0.0f && plane.normal.y < mover->minGroundNormalY) {
			plane.normal.y = 0.0f;
			plane.normal = b3Normalize(plane.normal);
		} else if (mover->flattenGround && plane.normal.y >= mover->minGroundNormalY) {
			plane.normal = {0.0f, 1.0f, 0.0f};
		}
		mover->planes.push_back({plane, FLT_MAX, 0.0f, true});

		const auto point = ToOu(results[i].point) + Vector3(static_cast<float>(mover->origin.x),
															static_cast<float>(mover->origin.y),
															static_cast<float>(mover->origin.z));
		mover->hits.push_back({b3StoreShapeId(shape), point, ToOu(results[i].plane.normal)});
	}

	return true;
}

bool FilterMover(b3ShapeId shape, void *context) {
	return !IsMoverIgnored(shape, *static_cast<MoverContext *>(context));
}

// ================================================================================================

} // namespace



void ou::PhysicsWorld::Initialize() {
	if (B3_IS_NON_NULL(sWorld)) { return; }

	b3SetLogFcn(LogImpl);
	b3SetAssertFcn(AssertImpl);

	auto def = b3DefaultWorldDef();
	def.gravity = kDefaultGravity;
	def.workerCount = std::clamp(std::thread::hardware_concurrency(), 1u, static_cast<uint32_t>(B3_MAX_WORKERS));
	def.frictionCallback = MixFriction;
	def.restitutionCallback = MixRestitution;
	sWorld = b3CreateWorld(&def);

	const auto version = b3GetVersion();
	Log::Info("Box3D {}.{}.{} を初期化しました (並列数 {})",
			  version.major, version.minor, version.revision, b3World_GetWorkerCount(sWorld));
}


void ou::PhysicsWorld::Finalize() {
	if (B3_IS_NULL(sWorld)) { return; }

	sActiveContacts.clear();
	sCenterOfMassOverrides.clear();

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
	sCenterOfMassOverrides.erase(id);
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


PhysicsWorld::ShapeId ou::PhysicsWorld::CreateShape(BodyId bodyId, const ShapeGeometry &geometry, const SurfaceMaterial &material, const CollisionFilter &filter, bool isSensor) {
	const auto body = FindBody(bodyId);
	if (B3_IS_NULL(body)) { return kInvalidShapeId; }

	auto def = b3DefaultShapeDef();
	def.isSensor = isSensor;
	def.density = isSensor ? 0.0f : 1.0f;
	def.updateBodyMass = false;
	def.enableContactEvents = true;
	def.enableSensorEvents = true;
	def.baseMaterial = ToB3(material);
	def.filter = ToB3(filter);

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


void ou::PhysicsWorld::SetShapeSurfaceMaterial(ShapeId id, const SurfaceMaterial &material) {
	const auto shape = FindShape(id);
	if (B3_IS_NULL(shape)) { return; }

	b3Shape_SetSurfaceMaterial(shape, ToB3(material));
}


void ou::PhysicsWorld::SetShapeFilter(ShapeId id, const CollisionFilter &filter) {
	const auto shape = FindShape(id);
	if (B3_IS_NULL(shape)) { return; }

	b3Shape_SetFilter(shape, ToB3(filter), true);
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


void ou::PhysicsWorld::SetBullet(BodyId id, bool isBullet) {
	const auto body = FindBody(id);
	if (B3_IS_NULL(body)) { return; }

	b3Body_SetBullet(body, isBullet);
}


void ou::PhysicsWorld::SetAwake(BodyId id, bool isAwake) {
	const auto body = FindBody(id);
	if (B3_IS_NULL(body)) { return; }

	b3Body_SetAwake(body, isAwake);
}


bool ou::PhysicsWorld::IsAwake(BodyId id) {
	const auto body = FindBody(id);
	if (B3_IS_NULL(body)) { return false; }

	return b3Body_IsAwake(body);
}


void ou::PhysicsWorld::SetSleepThreshold(BodyId id, float threshold) {
	const auto body = FindBody(id);
	if (B3_IS_NULL(body)) { return; }

	b3Body_SetSleepThreshold(body, Mathf::Max(threshold, 0.0f));
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


void ou::PhysicsWorld::AddForceAtPoint(BodyId id, const Vector3 &force, const Vector3 &point) {
	const auto body = FindBody(id);
	if (B3_IS_NULL(body)) { return; }

	b3Body_ApplyForce(body, ToB3(force), ToB3(point), true);
}


void ou::PhysicsWorld::AddImpulseAtPoint(BodyId id, const Vector3 &impulse, const Vector3 &point) {
	const auto body = FindBody(id);
	if (B3_IS_NULL(body)) { return; }

	b3Body_ApplyLinearImpulse(body, ToB3(impulse), ToB3(point), true);
}


Vector3 ou::PhysicsWorld::GetPointVelocity(BodyId id, const Vector3 &point) {
	const auto body = FindBody(id);
	if (B3_IS_NULL(body)) { return Vector3::Zero; }

	return ToOu(b3Body_GetWorldPointVelocity(body, ToB3(point)));
}


void ou::PhysicsWorld::SetLocalCenterOfMass(BodyId id, const Vector3 &center) {
	const auto body = FindBody(id);
	if (B3_IS_NULL(body)) { return; }

	sCenterOfMassOverrides[id] = ToB3(center);
	ApplyMass(body, b3Body_GetMass(body));
}


void ou::PhysicsWorld::ResetLocalCenterOfMass(BodyId id) {
	const auto body = FindBody(id);
	if (B3_IS_NULL(body)) { return; }

	sCenterOfMassOverrides.erase(id);
	ApplyMass(body, b3Body_GetMass(body));
}


Vector3 ou::PhysicsWorld::GetLocalCenterOfMass(BodyId id) {
	const auto body = FindBody(id);
	if (B3_IS_NULL(body)) { return Vector3::Zero; }

	return ToOu(b3Body_GetLocalCenterOfMass(body));
}


Vector3 ou::PhysicsWorld::GetWorldCenterOfMass(BodyId id) {
	const auto body = FindBody(id);
	if (B3_IS_NULL(body)) { return Vector3::Zero; }

	return ToOu(b3Body_GetWorldCenterOfMass(body));
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


std::vector<PhysicsWorld::RaycastHit> ou::PhysicsWorld::RaycastAll(const Vector3 &origin, const Vector3 &direction, float maxDistance, uint32_t layerMask, bool hitsSensors) {
	const auto length = direction.Length();
	if (B3_IS_NULL(sWorld) || length <= 0.0f || maxDistance <= 0.0f) { return {}; }

	RaycastContext raycast;
	raycast.distance = Mathf::Min(maxDistance, kMaxRaycastDistance);
	raycast.hitsSensors = hitsSensors;
	b3World_CastRay(sWorld, ToB3(origin), ToB3(direction / length * raycast.distance),
					MakeQueryFilter(layerMask), CollectRaycastHit, &raycast);

	std::sort(raycast.hits.begin(), raycast.hits.end(),
			  [](const RaycastHit &a, const RaycastHit &b) { return a.distance < b.distance; });
	return raycast.hits;
}


std::vector<PhysicsWorld::ShapeId> ou::PhysicsWorld::OverlapBox(const Vector3 &center, const Vector3 &halfExtents, const Quaternion &rotation, uint32_t layerMask, bool hitsSensors) {
	b3Vec3 corners[8];
	MakeBoxCorners(halfExtents, rotation, corners);

	return OverlapProxy(center, corners, 8, 0.0f, layerMask, hitsSensors);
}


std::vector<PhysicsWorld::ShapeId> ou::PhysicsWorld::OverlapSphere(const Vector3 &center, float radius, uint32_t layerMask, bool hitsSensors) {
	const b3Vec3 point = b3Vec3_zero;
	return OverlapProxy(center, &point, 1, Mathf::Abs(radius), layerMask, hitsSensors);
}


std::vector<PhysicsWorld::ShapeId> ou::PhysicsWorld::OverlapCapsule(const Vector3 &point0, const Vector3 &point1, float radius, uint32_t layerMask, bool hitsSensors) {
	const auto center = (point0 + point1) * 0.5f;
	const b3Vec3 points[2] = {ToB3(point0 - center),
							  ToB3(point1 - center)};
	return OverlapProxy(center, points, 2, Mathf::Abs(radius), layerMask, hitsSensors);
}


std::vector<PhysicsWorld::RaycastHit> ou::PhysicsWorld::BoxCastAll(const Vector3 &center, const Vector3 &halfExtents, const Quaternion &rotation, const Vector3 &direction, float maxDistance, uint32_t layerMask, bool hitsSensors) {
	b3Vec3 corners[8];
	MakeBoxCorners(halfExtents, rotation, corners);
	return CastProxy(center, corners, 8, 0.0f, direction, maxDistance, layerMask, hitsSensors);
}


std::vector<PhysicsWorld::RaycastHit> ou::PhysicsWorld::SphereCastAll(const Vector3 &origin, float radius, const Vector3 &direction, float maxDistance, uint32_t layerMask, bool hitsSensors) {
	const auto point = b3Vec3_zero;
	return CastProxy(origin, &point, 1, Mathf::Abs(radius), direction, maxDistance, layerMask, hitsSensors);
}


std::vector<PhysicsWorld::RaycastHit> ou::PhysicsWorld::CapsuleCastAll(const Vector3 &point0, const Vector3 &point1, float radius, const Vector3 &direction, float maxDistance, uint32_t layerMask, bool hitsSensors) {
	const auto center = (point0 + point1) * 0.5f;
	const b3Vec3 points[2] = {ToB3(point0 - center),
							  ToB3(point1 - center)};
	return CastProxy(center, points, 2, Mathf::Abs(radius), direction, maxDistance, layerMask, hitsSensors);
}


PhysicsWorld::CapsuleMoveResult ou::PhysicsWorld::MoveCapsule(const Vector3 &point0, const Vector3 &point1, float radius, const Vector3 &motion, uint32_t layerMask, ShapeId ignoreShape, float minGroundNormalY, bool flattenGround) {
	CapsuleMoveResult result;
	if (B3_IS_NULL(sWorld)) { return result; }

	const auto start = (point0 + point1) * 0.5f;
	const b3Capsule capsule = {ToB3(point0 - start), ToB3(point1 - start), Mathf::Abs(radius)};
	const auto filter = MakeQueryFilter(layerMask);
	const auto target = start + motion;

	MoverContext mover;
	mover.ignoreShape = FindShape(ignoreShape);
	mover.minGroundNormalY = minGroundNormalY;
	mover.flattenGround = flattenGround;

	auto position = start;
	for (int iteration = 0; iteration < kMoverIterations; iteration++) {
		mover.origin = ToB3(position);
		mover.planes.clear();
		b3World_CollideMover(sWorld, mover.origin, &capsule, filter, CollectMoverPlanes, &mover);
		const auto solved = b3SolvePlanes(ToB3(target - position), mover.planes.data(), static_cast<int>(mover.planes.size()));

		const auto fraction = b3World_CastMover(sWorld, mover.origin, &capsule, solved.delta, filter, FilterMover, &mover);
		const auto delta = ToOu(solved.delta) * fraction;
		position += delta;

		if (delta.LengthSquared() < kMoverTolerance * kMoverTolerance) { break; }
	}

	result.delta = position - start;
	result.hits = std::move(mover.hits);
	return result;
}
