#pragma once

#include "Math/Quaternion.h"
#include "Math/Vector3.h"

#include <cstdint>
#include <vector>


namespace ou::PhysicsWorld {

void Initialize();
void Finalize();
bool IsInitialized();

void Step(float deltaTime);


// ----- Body -----

using BodyId = uint64_t;
constexpr BodyId kInvalidBodyId = 0;

enum class MotionType {
	Static,
	Kinematic,
	Dynamic,
};

BodyId CreateBody(const Vector3 &position, const Quaternion &rotation, MotionType motionType);
void DestroyBody(BodyId id);

void SetBodyPose(BodyId id, const Vector3 &position, const Quaternion &rotation);
void GetBodyPose(BodyId id, Vector3 *position, Quaternion *rotation);

void SetMass(BodyId id, float mass);
void SetUseGravity(BodyId id, bool useGravity);
void SetMotionType(BodyId id, MotionType motionType);


// ----- Shape -----

using ShapeId = uint64_t;
constexpr ShapeId kInvalidShapeId = 0;

struct ShapeGeometry {
	enum class Type {
		Box,
		Sphere,
		Capsule,
	};

	Type type = Type::Box;
	Vector3 halfExtents = Vector3::Zero; // Box
	float radius = 0.0f;				 // Sphere / Capsule
	float height = 0.0f;				 // Capsule
	int direction = 1;					 // Capsule (0 = X, 1 = Y, 2 = Z)

	Vector3 localPosition = Vector3::Zero;
	Quaternion localRotation = Quaternion::Identity;
	Vector3 scale = Vector3::One;

	bool operator==(const ShapeGeometry &) const = default;
};

enum class CombineMode : uint8_t {
	Average,
	Minimum,
	Multiply,
	Maximum,
};

struct SurfaceMaterial {
	float friction = 0.6f;
	float restitution = 0.0f;
	CombineMode frictionCombine = CombineMode::Average;
	CombineMode restitutionCombine = CombineMode::Average;

	bool operator==(const SurfaceMaterial &) const = default;
};

struct CollisionFilter {
	uint32_t categoryBits = 1;
	uint32_t maskBits = 0xFFFFFFFF;

	bool operator==(const CollisionFilter &) const = default;
};

ShapeId CreateShape(BodyId bodyId, const ShapeGeometry &geometry, const SurfaceMaterial &material, const CollisionFilter &filter, bool isSensor);
void DestroyShape(ShapeId id);

void SetShapeGeometry(ShapeId id, const ShapeGeometry &geometry);
void SetShapeSurfaceMaterial(ShapeId id, const SurfaceMaterial &material);
void SetShapeFilter(ShapeId id, const CollisionFilter &filter);


// ----- Collision -----

struct ContactEvent {
	enum class Type {
		Enter,
		Stay,
		Exit,
	};

	Type type = Type::Enter;
	ShapeId shapeA = kInvalidShapeId;
	ShapeId shapeB = kInvalidShapeId;
	Vector3 normal = Vector3::Zero;
	float separation = 0.0f;
	std::vector<Vector3> points;
};

std::vector<ContactEvent> TakeContactEvents();

namespace DOF {
constexpr uint8_t TranslationX = 1 << 0;
constexpr uint8_t TranslationY = 1 << 1;
constexpr uint8_t TranslationZ = 1 << 2;
constexpr uint8_t RotationX = 1 << 3;
constexpr uint8_t RotationY = 1 << 4;
constexpr uint8_t RotationZ = 1 << 5;
constexpr uint8_t All = 0b111111;
} // namespace DOF

void SetAllowedDOFs(BodyId id, uint8_t dofs);
void SetLinearDamping(BodyId id, float damping);
void SetAngularDamping(BodyId id, float damping);


// ----- Force -----

void SetLinearVelocity(BodyId id, const Vector3 &velocity);
Vector3 GetLinearVelocity(BodyId id);
void SetAngularVelocity(BodyId id, const Vector3 &velocity);
Vector3 GetAngularVelocity(BodyId id);

void AddForce(BodyId id, const Vector3 &force);
void AddTorque(BodyId id, const Vector3 &torque);
void AddImpulse(BodyId id, const Vector3 &impulse);
void AddAngularImpulse(BodyId id, const Vector3 &impulse);

void MoveKinematic(BodyId id, const Vector3 &position, const Quaternion &rotation, float deltaTime);


// ----- World -----

void SetGravity(const Vector3 &gravity);
Vector3 GetGravity();


// ----- Raycast -----

struct RaycastHit {
	ShapeId shapeId = kInvalidShapeId;
	Vector3 point = Vector3::Zero;
	Vector3 normal = Vector3::Zero;
	float distance = 0.0f;
};

std::vector<RaycastHit> RaycastAll(const Vector3 &origin, const Vector3 &direction, float maxDistance, uint32_t layerMask, bool hitsSensors);
std::vector<ShapeId> OverlapBox(const Vector3 &center, const Vector3 &halfExtents, const Quaternion &rotation, uint32_t layerMask, bool hitsSensors);
std::vector<ShapeId> OverlapSphere(const Vector3 &center, float radius, uint32_t layerMask, bool hitsSensors);
std::vector<ShapeId> OverlapCapsule(const Vector3 &point0, const Vector3 &point1, float radius, uint32_t layerMask, bool hitsSensors);

} // namespace ou::PhysicsWorld
