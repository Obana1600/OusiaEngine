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


// ----- Shape -----

using ShapeId = uint32_t;
constexpr ShapeId kInvalidShapeId = 0xFFFFFFFF;

ShapeId CreateBoxShape(const Vector3 &halfExtents);
ShapeId CreateSphereShape(float radius);
void DestroyShape(ShapeId id);

void SetBoxHalfExtents(ShapeId id, const Vector3 &halfExtents);
void SetSphereRadius(ShapeId id, float radius);


// ----- Body -----

using BodyId = uint32_t;
constexpr BodyId kInvalidBodyId = 0xFFFFFFFF;

enum class MotionType {
	Static,
	Kinematic,
	Dynamic,
};

BodyId CreateBody(const Vector3 &position, const Quaternion &rotation, MotionType motionType);
void DestroyBody(BodyId id);

void SetBodyPose(BodyId id, const Vector3 &position, const Quaternion &rotation);
void GetBodyPose(BodyId id, Vector3 *position, Quaternion *rotation);

void AttachShape(BodyId bodyId, ShapeId shapeId,
				 const Vector3 &localPosition, const Quaternion &localRotation, const Vector3 &scale);
void DetachShape(ShapeId id);
void SetShapeLocalPose(ShapeId id,
					   const Vector3 &localPosition, const Quaternion &localRotation, const Vector3 &scale);

void SetMass(BodyId id, float mass);
void SetUseGravity(BodyId id, bool useGravity);
void SetMotionType(BodyId id, MotionType motionType);
void SetIsSensor(BodyId id, bool isSensor);


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


// ----- RayCast -----

struct RaycastHit {
	ShapeId shapeId = kInvalidShapeId;
	Vector3 point = Vector3::Zero;
	Vector3 normal = Vector3::Zero;
	float distance = 0.0f;
};

std::vector<RaycastHit> RaycastAll(const Vector3 &origin, const Vector3 &direction, float maxDistance, bool hitsSensors);
std::vector<ShapeId> OverlapSphere(const Vector3 &center, float radius, bool hitsSensors);
std::vector<ShapeId> OverlapBox(const Vector3 &center, const Vector3 &halfExtents, const Quaternion &rotation, bool hitsSensors);

} // namespace ou::PhysicsWorld
