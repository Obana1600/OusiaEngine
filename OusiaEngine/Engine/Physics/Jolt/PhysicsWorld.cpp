#include "PhysicsWorld.h"
#include "Diagnostics/Log.h"
#include "Math/Mathf.h"
#include "Physics/Jolt/JoltContactListener.h"
#include "Physics/Jolt/JoltLayers.h"

#include <Windows.h>
#include <algorithm>
#include <cmath>
#include <cstdarg>
#include <cstdio>
#include <memory>
#include <unordered_map>
#include <unordered_set>
#include <utility>

#include <Jolt/Core/Factory.h>
#include <Jolt/Core/JobSystemThreadPool.h>
#include <Jolt/Core/TempAllocator.h>
#include <Jolt/Physics/Body/BodyCreationSettings.h>
#include <Jolt/Physics/Body/BodyLock.h>
#include <Jolt/Physics/Collision/Shape/BoxShape.h>
#include <Jolt/Physics/Collision/Shape/EmptyShape.h>
#include <Jolt/Physics/Collision/Shape/MutableCompoundShape.h>
#include <Jolt/Physics/Collision/Shape/ScaledShape.h>
#include <Jolt/Physics/Collision/Shape/SphereShape.h>
#include <Jolt/Physics/PhysicsSettings.h>
#include <Jolt/Physics/PhysicsSystem.h>
#include <Jolt/RegisterTypes.h>

using namespace ou;


namespace {

// PhysicsSystem で使う定数
constexpr JPH::uint kMaxBodies = 1024;
constexpr JPH::uint kNumBodyMutexes = 0;
constexpr JPH::uint kMaxBodyPairs = 1024;
constexpr JPH::uint kMaxContactConstraints = 1024;

// 1 フレームの計算に使う一時的なメモリ
constexpr size_t kTempAllocatorSize = 10 * 1024 * 1024;

// 1 フレームがこの時間より長いとき衝突を何回かに分けて調べる
constexpr float kMaxCollisionStepTime = 1.0f / 60.0f;

// 形の大きさの下限
constexpr float kMinShapeExtent = 0.001f;

// 動かした Body の周りで起こす範囲の余白
constexpr float kWakeMargin = 0.1f;

// Shape 1 つぶん
struct ShapeEntry {
	JPH::RefConst<JPH::Shape> shape;
	PhysicsWorld::BodyId bodyId = PhysicsWorld::kInvalidBodyId;
	Vector3 localPosition = Vector3::Zero;
	Quaternion localRotation = Quaternion::Identity;
	Vector3 scale = Vector3::One;
};

// Body 1 つぶん
struct BodyEntry {
	std::vector<PhysicsWorld::ShapeId> shapeIds;
	float mass = 1.0f;
};

// Collision 1 つぶん
struct ContactState {
	PhysicsWorld::ShapeId shapeA = PhysicsWorld::kInvalidShapeId;
	PhysicsWorld::ShapeId shapeB = PhysicsWorld::kInvalidShapeId;
	bool isSleeping = false;
};

// グローバルなインスタンス
std::unique_ptr<JPH::TempAllocatorImpl> sTempAllocator;
std::unique_ptr<JPH::JobSystemThreadPool> sJobSystem;
std::unique_ptr<JPH::PhysicsSystem> sPhysicsSystem;

JoltLayers::BroadPhaseLayerInterface sBroadPhaseLayerInterface;
JoltLayers::ObjectVsBroadPhaseLayerFilter sObjectVsBroadPhaseLayerFilter;
JoltLayers::ObjectLayerPairFilter sObjectLayerPairFilter;

std::unordered_map<PhysicsWorld::ShapeId, ShapeEntry> sShapes;
std::unordered_map<PhysicsWorld::BodyId, BodyEntry> sBodies;
PhysicsWorld::ShapeId sNextShapeId = 0;

JoltContactListener sContactListener;
std::unordered_map<JPH::SubShapeIDPair, ContactState> sContacts;
std::vector<PhysicsWorld::ContactEvent> sContactEvents;


void TraceImpl(const char *format, ...) {
	char buffer[1024];
	va_list args;
	va_start(args, format);
	vsnprintf(buffer, sizeof(buffer), format, args);
	va_end(args);
	Log::Info("Jolt: {}", buffer);
}

#ifdef JPH_ENABLE_ASSERTS
bool AssertFailedImpl(const char *expression, const char *message, const char *file, JPH::uint line) {
	Log::Error("Jolt の assert: {} {} ({}:{})", expression, message ? message : "", file, line);
	return IsDebuggerPresent() != FALSE;
}
#endif


JPH::Vec3 ToJolt(const Vector3 &v) {
	return JPH::Vec3(v.x, v.y, v.z);
}

JPH::Quat ToJolt(const Quaternion &q) {
	return JPH::Quat(q.x, q.y, q.z, q.w).Normalized();
}

Vector3 ToOu(JPH::Vec3Arg v) {
	return Vector3(v.GetX(), v.GetY(), v.GetZ());
}

Quaternion ToOu(JPH::Quat q) {
	return Quaternion(q.GetX(), q.GetY(), q.GetZ(), q.GetW());
}

JPH::EMotionType ToJolt(PhysicsWorld::MotionType motionType) {
	switch (motionType) {
		case PhysicsWorld::MotionType::Kinematic:
			return JPH::EMotionType::Kinematic;
		case PhysicsWorld::MotionType::Dynamic:
			return JPH::EMotionType::Dynamic;
		default:
			return JPH::EMotionType::Static;
	}
}

JPH::ObjectLayer ToObjectLayer(PhysicsWorld::MotionType motionType) {
	return motionType == PhysicsWorld::MotionType::Static ? JoltLayers::Object::NonMoving
														  : JoltLayers::Object::Moving;
}

JPH::RefConst<JPH::Shape> CreateJoltBox(const Vector3 &halfExtents) {
	const auto extents = JPH::Vec3::sMax(ToJolt(halfExtents).Abs(), JPH::Vec3::sReplicate(kMinShapeExtent));
	return new JPH::BoxShape(extents, Mathf::Min(JPH::cDefaultConvexRadius, extents.ReduceMin()));
}

JPH::RefConst<JPH::Shape> CreateJoltSphere(float radius) {
	return new JPH::SphereShape(Mathf::Max(Mathf::Abs(radius), kMinShapeExtent));
}

JPH::RefConst<JPH::Shape> CreateScaledShape(const JPH::Shape *shape, const Vector3 &scale) {
	auto joltScale = ToJolt(scale).Abs();

	if (shape->GetSubType() == JPH::EShapeSubType::Sphere) {
		joltScale = JPH::Vec3::sReplicate(joltScale.ReduceMax());
	}
	joltScale = JPH::Vec3::sMax(joltScale, JPH::Vec3::sReplicate(kMinShapeExtent));

	if (joltScale.IsClose(JPH::Vec3::sOne())) { return shape; }
	return new JPH::ScaledShape(shape, joltScale);
}

void UpdateMassProperties(PhysicsWorld::BodyId id) {
	const auto it = sBodies.find(id);
	if (it == sBodies.end()) { return; }

	JPH::BodyLockWrite lock(sPhysicsSystem->GetBodyLockInterface(), JPH::BodyID(id));
	if (!lock.Succeeded()) { return; }

	auto &body = lock.GetBody();
	auto massProperties = body.GetShape()->GetMassProperties();
	massProperties.ScaleToMass(it->second.mass);
	body.GetMotionPropertiesUnchecked()->SetMassProperties(JPH::EAllowedDOFs::All, massProperties);
}

void RebuildBodyShape(PhysicsWorld::BodyId id) {
	const auto it = sBodies.find(id);
	if (it == sBodies.end()) { return; }

	JPH::RefConst<JPH::Shape> shape;
	if (it->second.shapeIds.empty()) {
		shape = new JPH::EmptyShape;
	} else {
		JPH::MutableCompoundShapeSettings settings;
		for (const auto shapeId : it->second.shapeIds) {
			const auto &entry = sShapes.at(shapeId);
			settings.AddShape(ToJolt(entry.localPosition),
							  ToJolt(entry.localRotation),
							  CreateScaledShape(entry.shape, entry.scale),
							  shapeId);
		}

		const auto result = settings.Create();
		if (result.HasError()) {
			Log::Error("{}", result.GetError().c_str());
			return;
		}
		shape = result.Get();
	}

	sPhysicsSystem->GetBodyInterface().SetShape(JPH::BodyID(id), shape, false, JPH::EActivation::Activate);
	UpdateMassProperties(id);
}

bool AreBodiesAdded(const JPH::SubShapeIDPair &key) {
	const auto &bodyInterface = sPhysicsSystem->GetBodyInterface();
	return bodyInterface.IsAdded(key.GetBody1ID()) && bodyInterface.IsAdded(key.GetBody2ID());
}

bool IsSleepingContact(const JPH::SubShapeIDPair &key) {
	const auto &bodyInterface = sPhysicsSystem->GetBodyInterface();
	auto hasMovingBody = false;
	for (const auto &id : {key.GetBody1ID(), key.GetBody2ID()}) {
		if (!bodyInterface.IsAdded(id)) { return false; }
		if (bodyInterface.GetMotionType(id) == JPH::EMotionType::Static) { continue; }
		if (bodyInterface.IsActive(id)) { return false; }
		hasMovingBody = true;
	}
	return hasMovingBody;
}

void PushExitEvent(const ContactState &state) {
	PhysicsWorld::ContactEvent event;
	event.type = PhysicsWorld::ContactEvent::Type::Exit;
	event.shapeA = state.shapeA;
	event.shapeB = state.shapeB;
	sContactEvents.push_back(std::move(event));
}

PhysicsWorld::ContactEvent MakeContactEvent(const JoltContactListener::Contact &contact, PhysicsWorld::ContactEvent::Type type) {
	PhysicsWorld::ContactEvent event;
	event.type = type;
	event.shapeA = contact.shapeA;
	event.shapeB = contact.shapeB;
	event.normal = ToOu(contact.normal);
	event.separation = -contact.penetrationDepth;
	for (const auto &point : contact.points) {
		event.points.push_back(ToOu(JPH::Vec3(point)));
	}
	return event;
}

void CollectContactEvents() {
	std::vector<JoltContactListener::Contact> contacts;
	sContactListener.TakeContacts(&contacts);

	std::unordered_map<JPH::SubShapeIDPair, const JoltContactListener::Contact *> latest;
	for (const auto &contact : contacts) {
		latest[contact.key] = &contact;
	}

	for (const auto &contact : contacts) {
		if (latest.at(contact.key) != &contact) { continue; }

		if (!contact.isRemoved) {
			const auto [it, isNew] = sContacts.try_emplace(contact.key, ContactState(contact.shapeA, contact.shapeB));
			it->second.isSleeping = false;

			sContactEvents.push_back(MakeContactEvent(
				contact,
				isNew ? PhysicsWorld::ContactEvent::Type::Enter
					  : PhysicsWorld::ContactEvent::Type::Stay));
			continue;
		}


		const auto it = sContacts.find(contact.key);
		if (it == sContacts.end()) { continue; }

		if (IsSleepingContact(contact.key)) {
			it->second.isSleeping = true;
			continue;
		}

		if (AreBodiesAdded(contact.key)) { PushExitEvent(it->second); }
		sContacts.erase(it);
	}

	std::erase_if(sContacts, [](const auto &pair) {
		const auto &[key, state] = pair;
		if (!state.isSleeping || IsSleepingContact(key)) { return false; }

		if (AreBodiesAdded(key)) { PushExitEvent(state); }
		return true;
	});
}

} // namespace



void ou::PhysicsWorld::Initialize() {
	if (sPhysicsSystem) { return; }

	JPH::RegisterDefaultAllocator();
	JPH::Trace = TraceImpl;
	JPH_IF_ENABLE_ASSERTS(JPH::AssertFailed = AssertFailedImpl;)
	JPH::Factory::sInstance = new JPH::Factory;
	JPH::RegisterTypes();

	sTempAllocator = std::make_unique<JPH::TempAllocatorImpl>(kTempAllocatorSize);
	sJobSystem = std::make_unique<JPH::JobSystemThreadPool>(JPH::cMaxPhysicsJobs, JPH::cMaxPhysicsBarriers, -1);

	sPhysicsSystem = std::make_unique<JPH::PhysicsSystem>();
	sPhysicsSystem->Init(kMaxBodies, kNumBodyMutexes, kMaxBodyPairs, kMaxContactConstraints,
						 sBroadPhaseLayerInterface, sObjectVsBroadPhaseLayerFilter, sObjectLayerPairFilter);

	sPhysicsSystem->SetContactListener(&sContactListener);

	Log::Info("Jolt Physics {}.{}.{} を初期化しました (並列数 {})",
			  JPH_VERSION_MAJOR, JPH_VERSION_MINOR, JPH_VERSION_PATCH, sJobSystem->GetMaxConcurrency());
}


void ou::PhysicsWorld::Finalize() {
	if (!sPhysicsSystem) { return; }

	sPhysicsSystem->SetContactListener(nullptr);
	sContacts.clear();
	sContactEvents.clear();

	sShapes.clear();
	sBodies.clear();

	sPhysicsSystem.reset();
	sJobSystem.reset();
	sTempAllocator.reset();

	JPH::UnregisterTypes();
	delete JPH::Factory::sInstance;
	JPH::Factory::sInstance = nullptr;
}


bool ou::PhysicsWorld::IsInitialized() {
	return sPhysicsSystem != nullptr;
}


void ou::PhysicsWorld::Step(float deltaTime) {
	if (!sPhysicsSystem || deltaTime <= 0.0f) { return; }

	const auto collisionSteps = std::max(1, static_cast<int>(std::ceil(deltaTime / kMaxCollisionStepTime)));
	const auto error = sPhysicsSystem->Update(deltaTime, collisionSteps, sTempAllocator.get(), sJobSystem.get());

	if (error != JPH::EPhysicsUpdateError::None) {
		Log::Warning("物理の更新で一部の接触が無効になりました: 0x{:X}", static_cast<uint32_t>(error));
	}

	CollectContactEvents();
}


PhysicsWorld::ShapeId ou::PhysicsWorld::CreateBoxShape(const Vector3 &halfExtents) {
	const auto id = sNextShapeId++;
	sShapes[id].shape = CreateJoltBox(halfExtents);
	return id;
}


PhysicsWorld::ShapeId ou::PhysicsWorld::CreateSphereShape(float radius) {
	const auto id = sNextShapeId++;
	sShapes[id].shape = CreateJoltSphere(radius);
	return id;
}


void ou::PhysicsWorld::DestroyShape(ShapeId id) {
	DetachShape(id);
	sShapes.erase(id);
}


void ou::PhysicsWorld::SetBoxHalfExtents(ShapeId id, const Vector3 &halfExtents) {
	const auto it = sShapes.find(id);
	if (it == sShapes.end()) { return; }
	if (it->second.shape->GetSubType() != JPH::EShapeSubType::Box) {
		Log::Warning("Box でないコライダーに SetBoxHalfExtents が呼ばれました");
		return;
	}

	it->second.shape = CreateJoltBox(halfExtents);
	RebuildBodyShape(it->second.bodyId);
}


void ou::PhysicsWorld::SetSphereRadius(ShapeId id, float radius) {
	const auto it = sShapes.find(id);
	if (it == sShapes.end()) { return; }
	if (it->second.shape->GetSubType() != JPH::EShapeSubType::Sphere) {
		Log::Warning("Sphere でないコライダーに SetSphereRadius が呼ばれました");
		return;
	}

	it->second.shape = CreateJoltSphere(radius);
	RebuildBodyShape(it->second.bodyId);
}


PhysicsWorld::BodyId ou::PhysicsWorld::CreateBody(const Vector3 &position, const Quaternion &rotation, MotionType motionType) {
	if (!sPhysicsSystem) { return kInvalidBodyId; }

	JPH::BodyCreationSettings settings(
		new JPH::EmptyShape,
		JPH::RVec3(ToJolt(position)),
		ToJolt(rotation),
		ToJolt(motionType),
		ToObjectLayer(motionType));
	settings.mAllowDynamicOrKinematic = true;
	settings.mUseManifoldReduction = false;

	auto &bodyInterface = sPhysicsSystem->GetBodyInterface();
	const auto id = bodyInterface.CreateAndAddBody(
		settings,
		motionType == MotionType::Static ? JPH::EActivation::DontActivate
										 : JPH::EActivation::Activate);
	if (id.IsInvalid()) {
		Log::Error("Body の作成に失敗しました (最大 {} 個)", kMaxBodies);
		return kInvalidBodyId;
	}

	const auto bodyId = id.GetIndexAndSequenceNumber();
	sBodies[bodyId] = {};
	return bodyId;
}


void ou::PhysicsWorld::DestroyBody(BodyId id) {
	if (!sPhysicsSystem) { return; }

	const auto it = sBodies.find(id);
	if (it == sBodies.end()) { return; }

	for (const auto shapeId : it->second.shapeIds) {
		sShapes.at(shapeId).bodyId = kInvalidBodyId;
	}
	sBodies.erase(it);

	auto &bodyInterface = sPhysicsSystem->GetBodyInterface();
	const JPH::BodyID bodyId(id);
	bodyInterface.RemoveBody(bodyId);
	bodyInterface.DestroyBody(bodyId);
}


void ou::PhysicsWorld::SetBodyPose(BodyId id, const Vector3 &position, const Quaternion &rotation) {
	if (!sPhysicsSystem || id == kInvalidBodyId) { return; }

	auto &bodyInterface = sPhysicsSystem->GetBodyInterface();
	const JPH::BodyID bodyId(id);

	auto bounds = bodyInterface.GetTransformedShape(bodyId).GetWorldSpaceBounds();
	bodyInterface.SetPositionAndRotation(bodyId, JPH::RVec3(ToJolt(position)), ToJolt(rotation), JPH::EActivation::Activate);
	bounds.Encapsulate(bodyInterface.GetTransformedShape(bodyId).GetWorldSpaceBounds());
	bounds.ExpandBy(JPH::Vec3::sReplicate(kWakeMargin));
	bodyInterface.ActivateBodiesInAABox(bounds, JPH::BroadPhaseLayerFilter(), JPH::ObjectLayerFilter());
}


void ou::PhysicsWorld::GetBodyPose(BodyId id, Vector3 *position, Quaternion *rotation) {
	if (!sPhysicsSystem || id == kInvalidBodyId) { return; }

	JPH::RVec3 joltPosition;
	JPH::Quat joltRotation;
	sPhysicsSystem->GetBodyInterface().GetPositionAndRotation(JPH::BodyID(id), joltPosition, joltRotation);
	if (position) { *position = ToOu(JPH::Vec3(joltPosition)); }
	if (rotation) { *rotation = ToOu(joltRotation); }
}


void ou::PhysicsWorld::AttachShape(BodyId bodyId, ShapeId shapeId, const Vector3 &localPosition, const Quaternion &localRotation, const Vector3 &scale) {
	const auto shapeIt = sShapes.find(shapeId);
	const auto bodyIt = sBodies.find(bodyId);
	if (shapeIt == sShapes.end() || bodyIt == sBodies.end()) { return; }

	DetachShape(shapeId);

	auto &entry = shapeIt->second;
	entry.bodyId = bodyId;
	entry.localPosition = localPosition;
	entry.localRotation = localRotation;
	entry.scale = scale;
	bodyIt->second.shapeIds.push_back(shapeId);

	RebuildBodyShape(bodyId);
}


void ou::PhysicsWorld::DetachShape(ShapeId id) {
	const auto it = sShapes.find(id);
	if (it == sShapes.end() || it->second.bodyId == kInvalidBodyId) { return; }

	const auto bodyId = it->second.bodyId;
	it->second.bodyId = kInvalidBodyId;
	std::erase(sBodies.at(bodyId).shapeIds, id);

	RebuildBodyShape(bodyId);
}


void ou::PhysicsWorld::SetShapeLocalPose(ShapeId id, const Vector3 &localPosition, const Quaternion &localRotation, const Vector3 &scale) {
	const auto it = sShapes.find(id);
	if (it == sShapes.end()) { return; }

	auto &entry = it->second;
	entry.localPosition = localPosition;
	entry.localRotation = localRotation;
	entry.scale = scale;

	RebuildBodyShape(entry.bodyId);
}


void ou::PhysicsWorld::SetMass(BodyId id, float mass) {
	const auto it = sBodies.find(id);
	if (it == sBodies.end()) { return; }

	it->second.mass = mass;
	UpdateMassProperties(id);
}


void ou::PhysicsWorld::SetUseGravity(BodyId id, bool useGravity) {
	if (!sPhysicsSystem || id == kInvalidBodyId) { return; }

	sPhysicsSystem->GetBodyInterface().SetGravityFactor(JPH::BodyID(id), useGravity ? 1.0f : 0.0f);
}


void ou::PhysicsWorld::SetMotionType(BodyId id, MotionType motionType) {
	if (!sPhysicsSystem || id == kInvalidBodyId) { return; }

	auto &bodyInterface = sPhysicsSystem->GetBodyInterface();
	const JPH::BodyID bodyId(id);
	bodyInterface.SetObjectLayer(bodyId, ToObjectLayer(motionType));
	bodyInterface.SetMotionType(bodyId, ToJolt(motionType), JPH::EActivation::Activate);
}


void ou::PhysicsWorld::SetIsSensor(BodyId id, bool isSensor) {
	if (!sPhysicsSystem || id == kInvalidBodyId) { return; }

	JPH::BodyLockWrite lock(sPhysicsSystem->GetBodyLockInterface(), JPH::BodyID(id));
	if (!lock.Succeeded()) { return; }

	auto &body = lock.GetBody();
	body.SetIsSensor(isSensor);
	body.SetCollideKinematicVsNonDynamic(isSensor);
}


std::vector<PhysicsWorld::ContactEvent> ou::PhysicsWorld::TakeContactEvents() {
	return std::exchange(sContactEvents, {});
}
