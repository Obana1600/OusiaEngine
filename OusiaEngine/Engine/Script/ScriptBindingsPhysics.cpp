#include "ScriptBindings.h"
#include "Math/Mathf.h"
#include "Math/Quaternion.h"
#include "Math/Vector3.h"
#include "Physics/BoxCollider.h"
#include "Physics/CapsuleCollider.h"
#include "Physics/CharacterController.h"
#include "Physics/Collider.h"
#include "Physics/Collision.h"
#include "Physics/Physics.h"
#include "Physics/RaycastHit.h"
#include "Physics/Rigidbody.h"
#include "Physics/SphereCollider.h"
#include "Scene/GameObject.h"
#include "Scene/Transform.h"
#include "Script/ScriptBindingsInternal.h"

#include <cstdint>
#include <format>
#include <new>
#include <vector>

#include <angelscript.h>

using namespace ou;
using namespace ou::ScriptBindingsInternal;


namespace {

// ---- スクリプトに渡す値の型 ----
// C++ の Collision などは Collider * を持つが、スクリプトはあとまで覚えておけるので、instanceId で持つ

struct ScriptContactPoint {
	Vector3 point;
	Vector3 normal;
	float separation = 0.0f;
	uint32_t thisColliderId = 0;
	uint32_t otherColliderId = 0;
};

struct ScriptCollision {
	uint32_t colliderId = 0;
	uint32_t rigidbodyId = 0;
	Vector3 relativeVelocity;
	Vector3 impulse;
	std::vector<ScriptContactPoint> contacts;
};

struct ScriptRaycastHit {
	Vector3 point;
	Vector3 normal;
	float distance = 0.0f;
	uint32_t colliderId = 0;
};

uint32_t IdOf(const Object *object) {
	return object ? object->GetInstanceId() : 0;
}

// 消えていたら null（例外にはしない。Unity でも消えた相手の collider は null になる）
template <typename T>
T *FindById(uint32_t instanceId) {
	return instanceId ? dynamic_cast<T *>(Object::FindObjectFromInstanceId(instanceId)) : nullptr;
}

template <typename T>
ObjectHandle *HandleById(uint32_t instanceId) {
	return MakeHandle(FindById<T>(instanceId));
}


// ---- ContactPoint ----

void ConstructContactPoint(void *memory) { new (memory) ScriptContactPoint(); }
ObjectHandle *GetThisCollider(const ScriptContactPoint *self) { return HandleById<Collider>(self->thisColliderId); }
ObjectHandle *GetOtherCollider(const ScriptContactPoint *self) { return HandleById<Collider>(self->otherColliderId); }


// ---- Collision ----

void ConstructCollision(void *memory) { new (memory) ScriptCollision(); }
void CopyConstructCollision(const ScriptCollision &other, void *memory) { new (memory) ScriptCollision(other); }
void DestructCollision(ScriptCollision *self) { self->~ScriptCollision(); }
ScriptCollision &AssignCollision(const ScriptCollision &other, ScriptCollision *self) { return *self = other; }

ObjectHandle *GetCollisionCollider(const ScriptCollision *self) { return HandleById<Collider>(self->colliderId); }
ObjectHandle *GetCollisionRigidbody(const ScriptCollision *self) { return HandleById<Rigidbody>(self->rigidbodyId); }

// Unity と同じく、相手に Rigidbody があればその GameObject、なければ Collider の GameObject
Component *CollisionOwner(const ScriptCollision *self) {
	if (auto *rigidbody = FindById<Rigidbody>(self->rigidbodyId)) { return rigidbody; }
	return FindById<Collider>(self->colliderId);
}

ObjectHandle *GetCollisionGameObject(const ScriptCollision *self) {
	auto *owner = CollisionOwner(self);
	return owner ? MakeHandle(owner->GetGameObject()) : nullptr;
}

ObjectHandle *GetCollisionTransform(const ScriptCollision *self) {
	auto *owner = CollisionOwner(self);
	return owner ? MakeHandle(owner->GetTransform()) : nullptr;
}

Vector3 GetRelativeVelocity(const ScriptCollision *self) { return self->relativeVelocity; }
Vector3 GetImpulse(const ScriptCollision *self) { return self->impulse; }
int GetContactCount(const ScriptCollision *self) { return static_cast<int>(self->contacts.size()); }

ScriptContactPoint GetContact(int index, const ScriptCollision *self) {
	if (index < 0 || index >= static_cast<int>(self->contacts.size())) {
		ThrowException(std::format("IndexOutOfRangeException: GetContact({}) (contactCount は {})", index, self->contacts.size()));
		return {};
	}
	return self->contacts[index];
}


// ---- RaycastHit ----

void ConstructRaycastHit(void *memory) { new (memory) ScriptRaycastHit(); }
ObjectHandle *GetHitCollider(const ScriptRaycastHit *self) { return HandleById<Collider>(self->colliderId); }

ObjectHandle *GetHitRigidbody(const ScriptRaycastHit *self) {
	auto *collider = FindById<Collider>(self->colliderId);
	return collider ? MakeHandle(collider->GetAttachedRigidbody()) : nullptr;
}

ObjectHandle *GetHitTransform(const ScriptRaycastHit *self) {
	auto *collider = FindById<Collider>(self->colliderId);
	if (!collider) { return nullptr; }
	if (auto *rigidbody = collider->GetAttachedRigidbody()) { return MakeHandle(rigidbody->GetTransform()); }
	return MakeHandle(collider->GetTransform());
}


// ---- Physics ----

const int kDefaultRaycastLayers = static_cast<int>(Physics::kDefaultRaycastLayers);
const int kAllLayers = static_cast<int>(Physics::kAllLayers);

bool Raycast(const Vector3 &origin, const Vector3 &direction, float maxDistance, int layerMask) {
	return Physics::Raycast(origin, direction, nullptr, maxDistance, static_cast<uint32_t>(layerMask));
}

bool RaycastHitInfo(const Vector3 &origin, const Vector3 &direction, ScriptRaycastHit &hitInfo, float maxDistance, int layerMask) {
	RaycastHit hit;
	const auto isHit = Physics::Raycast(origin, direction, &hit, maxDistance, static_cast<uint32_t>(layerMask));
	hitInfo = isHit ? ScriptRaycastHit{hit.point, hit.normal, hit.distance, IdOf(hit.collider)} : ScriptRaycastHit{};
	return isHit;
}


// ---- Rigidbody（ForceMode は AngelScript の enum なので int で受ける） ----

template <void (Rigidbody::*Function)(const Vector3 &, ForceMode)>
void AddForceWithMode(const Vector3 &value, int mode, ObjectHandle *self) {
	if (auto *rigidbody = Resolve<Rigidbody>(self)) { (rigidbody->*Function)(value, static_cast<ForceMode>(mode)); }
}

void AddForceAtPosition(const Vector3 &force, const Vector3 &position, int mode, ObjectHandle *self) {
	if (auto *rigidbody = Resolve<Rigidbody>(self)) { rigidbody->AddForceAtPosition(force, position, static_cast<ForceMode>(mode)); }
}

template <void (Rigidbody::*Function)()>
void CallRigidbody(ObjectHandle *self) {
	if (auto *rigidbody = Resolve<Rigidbody>(self)) { (rigidbody->*Function)(); }
}


// ---- Collider ----

Vector3 ClosestPoint(const Vector3 &position, ObjectHandle *self) {
	auto *collider = Resolve<Collider>(self);
	return collider ? collider->ClosestPoint(position) : position;
}

void RegisterColliderMembers(const TypeRegistrar &c) {
	RegisterBehaviorMembers(c);
	c.Method("bool get_isTrigger() const property", asFUNCTION((Get<Collider, bool, &Collider::IsTrigger>)));
	c.Method("void set_isTrigger(bool value) property", asFUNCTION((Set<Collider, bool, &Collider::SetIsTrigger>)));
	c.Method("Rigidbody@ get_attachedRigidbody() const property", asFUNCTION((GetHandle<Collider, Rigidbody, &Collider::GetAttachedRigidbody>)));
	c.Method("Vector3 ClosestPoint(const Vector3 &in position) const", asFUNCTION(ClosestPoint));
}


// ---- CharacterController ----

// Unity と同じく CollisionFlags を返す（Sides・Above・Below のビット）
int Move(const Vector3 &motion, ObjectHandle *self) {
	auto *controller = Resolve<CharacterController>(self);
	return controller ? static_cast<int>(controller->Move(motion)) : 0;
}

bool SimpleMove(const Vector3 &speed, ObjectHandle *self) {
	auto *controller = Resolve<CharacterController>(self);
	return controller && controller->SimpleMove(speed);
}

} // namespace



void *ou::ScriptBindings::CreateCollision(const Collision &collision) {
	auto *script = new ScriptCollision();
	script->colliderId = IdOf(collision.GetCollider());
	script->rigidbodyId = IdOf(collision.GetRigidbody());
	script->relativeVelocity = collision.GetRelativeVelocity();
	script->impulse = collision.GetImpulse();
	for (const auto &contact : collision.GetContacts()) {
		script->contacts.push_back({contact.point, contact.normal, contact.separation, IdOf(contact.thisCollider), IdOf(contact.otherCollider)});
	}
	return script;
}


void ou::ScriptBindings::DestroyCollision(void *collision) {
	delete static_cast<ScriptCollision *>(collision);
}


void ou::ScriptBindings::RegisterPhysicsTypes(asIScriptEngine *engine) {
	Check(engine->RegisterObjectType("ContactPoint", sizeof(ScriptContactPoint), asOBJ_VALUE | asOBJ_POD | asGetTypeTraits<ScriptContactPoint>()), "ContactPoint");
	Check(engine->RegisterObjectType("RaycastHit", sizeof(ScriptRaycastHit), asOBJ_VALUE | asOBJ_POD | asGetTypeTraits<ScriptRaycastHit>()), "RaycastHit");
	// Collision は接触点の配列を持つので、作る・コピー・消すを登録する
	Check(engine->RegisterObjectType("Collision", sizeof(ScriptCollision), asOBJ_VALUE | asGetTypeTraits<ScriptCollision>()), "Collision");

	Check(engine->RegisterEnum("ForceMode"), "ForceMode");
	Check(engine->RegisterEnumValue("ForceMode", "Force", static_cast<int>(ForceMode::Force)), "ForceMode::Force");
	Check(engine->RegisterEnumValue("ForceMode", "Acceleration", static_cast<int>(ForceMode::Acceleration)), "ForceMode::Acceleration");
	Check(engine->RegisterEnumValue("ForceMode", "Impulse", static_cast<int>(ForceMode::Impulse)), "ForceMode::Impulse");
	Check(engine->RegisterEnumValue("ForceMode", "VelocityChange", static_cast<int>(ForceMode::VelocityChange)), "ForceMode::VelocityChange");

	// CharacterController.Move が返すビット（Unity と同じ）
	Check(engine->RegisterEnum("CollisionFlags"), "CollisionFlags");
	Check(engine->RegisterEnumValue("CollisionFlags", "None", static_cast<int>(CollisionFlags::None)), "CollisionFlags::None");
	Check(engine->RegisterEnumValue("CollisionFlags", "Sides", static_cast<int>(CollisionFlags::Sides)), "CollisionFlags::Sides");
	Check(engine->RegisterEnumValue("CollisionFlags", "Above", static_cast<int>(CollisionFlags::Above)), "CollisionFlags::Above");
	Check(engine->RegisterEnumValue("CollisionFlags", "Below", static_cast<int>(CollisionFlags::Below)), "CollisionFlags::Below");
}


void ou::ScriptBindings::RegisterPhysicsMembers(asIScriptEngine *engine) {
	const auto method = [&](const char *type, const char *declaration, const asSFuncPtr &function) {
		Check(engine->RegisterObjectMethod(type, declaration, function, asCALL_CDECL_OBJLAST), declaration);
	};
	const auto behaviour = [&](const char *type, asEBehaviours kind, const char *declaration, const asSFuncPtr &function) {
		Check(engine->RegisterObjectBehaviour(type, kind, declaration, function, asCALL_CDECL_OBJLAST), declaration);
	};
	const auto property = [&](const char *type, const char *declaration, int offset) {
		Check(engine->RegisterObjectProperty(type, declaration, offset), declaration);
	};

	// ---- ContactPoint ----
	behaviour("ContactPoint", asBEHAVE_CONSTRUCT, "void f()", asFUNCTION(ConstructContactPoint));
	property("ContactPoint", "Vector3 point", asOFFSET(ScriptContactPoint, point));
	property("ContactPoint", "Vector3 normal", asOFFSET(ScriptContactPoint, normal));
	property("ContactPoint", "float separation", asOFFSET(ScriptContactPoint, separation));
	method("ContactPoint", "Collider@ get_thisCollider() const property", asFUNCTION(GetThisCollider));
	method("ContactPoint", "Collider@ get_otherCollider() const property", asFUNCTION(GetOtherCollider));

	// ---- Collision ----
	behaviour("Collision", asBEHAVE_CONSTRUCT, "void f()", asFUNCTION(ConstructCollision));
	behaviour("Collision", asBEHAVE_CONSTRUCT, "void f(const Collision &in other)", asFUNCTION(CopyConstructCollision));
	behaviour("Collision", asBEHAVE_DESTRUCT, "void f()", asFUNCTION(DestructCollision));
	method("Collision", "Collision &opAssign(const Collision &in other)", asFUNCTION(AssignCollision));
	method("Collision", "GameObject@ get_gameObject() const property", asFUNCTION(GetCollisionGameObject));
	method("Collision", "Transform@ get_transform() const property", asFUNCTION(GetCollisionTransform));
	method("Collision", "Collider@ get_collider() const property", asFUNCTION(GetCollisionCollider));
	method("Collision", "Rigidbody@ get_rigidbody() const property", asFUNCTION(GetCollisionRigidbody));
	method("Collision", "Vector3 get_relativeVelocity() const property", asFUNCTION(GetRelativeVelocity));
	method("Collision", "Vector3 get_impulse() const property", asFUNCTION(GetImpulse));
	method("Collision", "int get_contactCount() const property", asFUNCTION(GetContactCount));
	method("Collision", "ContactPoint GetContact(int index) const", asFUNCTION(GetContact));

	// ---- RaycastHit ----
	behaviour("RaycastHit", asBEHAVE_CONSTRUCT, "void f()", asFUNCTION(ConstructRaycastHit));
	property("RaycastHit", "Vector3 point", asOFFSET(ScriptRaycastHit, point));
	property("RaycastHit", "Vector3 normal", asOFFSET(ScriptRaycastHit, normal));
	property("RaycastHit", "float distance", asOFFSET(ScriptRaycastHit, distance));
	method("RaycastHit", "Collider@ get_collider() const property", asFUNCTION(GetHitCollider));
	method("RaycastHit", "Rigidbody@ get_rigidbody() const property", asFUNCTION(GetHitRigidbody));
	method("RaycastHit", "Transform@ get_transform() const property", asFUNCTION(GetHitTransform));

	// ---- Collider ----
	{
		RegisterColliderMembers(RegisterComponentType(engine, "Collider"));
	}

	// ---- BoxCollider ----
	{
		const auto c = RegisterComponentType(engine, "BoxCollider");
		RegisterColliderMembers(c);
		c.Method("Vector3 get_center() const property", asFUNCTION((Get<BoxCollider, const Vector3 &, &BoxCollider::GetCenter>)));
		c.Method("void set_center(const Vector3 &in value) property", asFUNCTION((Set<BoxCollider, const Vector3 &, &BoxCollider::SetCenter>)));
		c.Method("Vector3 get_size() const property", asFUNCTION((Get<BoxCollider, const Vector3 &, &BoxCollider::GetSize>)));
		c.Method("void set_size(const Vector3 &in value) property", asFUNCTION((Set<BoxCollider, const Vector3 &, &BoxCollider::SetSize>)));
	}

	// ---- SphereCollider ----
	{
		const auto c = RegisterComponentType(engine, "SphereCollider");
		RegisterColliderMembers(c);
		c.Method("Vector3 get_center() const property", asFUNCTION((Get<SphereCollider, const Vector3 &, &SphereCollider::GetCenter>)));
		c.Method("void set_center(const Vector3 &in value) property", asFUNCTION((Set<SphereCollider, const Vector3 &, &SphereCollider::SetCenter>)));
		c.Method("float get_radius() const property", asFUNCTION((Get<SphereCollider, float, &SphereCollider::GetRadius>)));
		c.Method("void set_radius(float value) property", asFUNCTION((Set<SphereCollider, float, &SphereCollider::SetRadius>)));
	}

	// ---- CapsuleCollider ----
	{
		const auto c = RegisterComponentType(engine, "CapsuleCollider");
		RegisterColliderMembers(c);
		c.Method("Vector3 get_center() const property", asFUNCTION((Get<CapsuleCollider, const Vector3 &, &CapsuleCollider::GetCenter>)));
		c.Method("void set_center(const Vector3 &in value) property", asFUNCTION((Set<CapsuleCollider, const Vector3 &, &CapsuleCollider::SetCenter>)));
		c.Method("float get_radius() const property", asFUNCTION((Get<CapsuleCollider, float, &CapsuleCollider::GetRadius>)));
		c.Method("void set_radius(float value) property", asFUNCTION((Set<CapsuleCollider, float, &CapsuleCollider::SetRadius>)));
		c.Method("float get_height() const property", asFUNCTION((Get<CapsuleCollider, float, &CapsuleCollider::GetHeight>)));
		c.Method("void set_height(float value) property", asFUNCTION((Set<CapsuleCollider, float, &CapsuleCollider::SetHeight>)));
		// Unity と同じく 0: X、1: Y、2: Z
		c.Method("int get_direction() const property", asFUNCTION((Get<CapsuleCollider, int, &CapsuleCollider::GetDirection>)));
		c.Method("void set_direction(int value) property", asFUNCTION((Set<CapsuleCollider, int, &CapsuleCollider::SetDirection>)));
	}

	// ---- CharacterController ----
	{
		const auto c = RegisterComponentType(engine, "CharacterController");
		RegisterColliderMembers(c);
		c.Method("CollisionFlags Move(const Vector3 &in motion)", asFUNCTION(Move));
		c.Method("bool SimpleMove(const Vector3 &in speed)", asFUNCTION(SimpleMove));
		c.Method("bool get_isGrounded() const property", asFUNCTION((Get<CharacterController, bool, &CharacterController::IsGrounded>)));
		c.Method("Vector3 get_velocity() const property", asFUNCTION((Get<CharacterController, const Vector3 &, &CharacterController::GetVelocity>)));
		c.Method("Vector3 get_center() const property", asFUNCTION((Get<CharacterController, const Vector3 &, &CharacterController::GetCenter>)));
		c.Method("void set_center(const Vector3 &in value) property", asFUNCTION((Set<CharacterController, const Vector3 &, &CharacterController::SetCenter>)));
		c.Method("float get_radius() const property", asFUNCTION((Get<CharacterController, float, &CharacterController::GetRadius>)));
		c.Method("void set_radius(float value) property", asFUNCTION((Set<CharacterController, float, &CharacterController::SetRadius>)));
		c.Method("float get_height() const property", asFUNCTION((Get<CharacterController, float, &CharacterController::GetHeight>)));
		c.Method("void set_height(float value) property", asFUNCTION((Set<CharacterController, float, &CharacterController::SetHeight>)));
		c.Method("float get_slopeLimit() const property", asFUNCTION((Get<CharacterController, float, &CharacterController::GetSlopeLimit>)));
		c.Method("void set_slopeLimit(float value) property", asFUNCTION((Set<CharacterController, float, &CharacterController::SetSlopeLimit>)));
		c.Method("float get_stepOffset() const property", asFUNCTION((Get<CharacterController, float, &CharacterController::GetStepOffset>)));
		c.Method("void set_stepOffset(float value) property", asFUNCTION((Set<CharacterController, float, &CharacterController::SetStepOffset>)));
	}

	// ---- Rigidbody ----
	{
		const auto r = RegisterComponentType(engine, "Rigidbody");
		r.Method("float get_mass() const property", asFUNCTION((Get<Rigidbody, float, &Rigidbody::GetMass>)));
		r.Method("void set_mass(float value) property", asFUNCTION((Set<Rigidbody, float, &Rigidbody::SetMass>)));
		r.Method("bool get_useGravity() const property", asFUNCTION((Get<Rigidbody, bool, &Rigidbody::GetUseGravity>)));
		r.Method("void set_useGravity(bool value) property", asFUNCTION((Set<Rigidbody, bool, &Rigidbody::SetUseGravity>)));
		r.Method("bool get_isKinematic() const property", asFUNCTION((Get<Rigidbody, bool, &Rigidbody::IsKinematic>)));
		r.Method("void set_isKinematic(bool value) property", asFUNCTION((Set<Rigidbody, bool, &Rigidbody::SetIsKinematic>)));
		r.Method("float get_linearDamping() const property", asFUNCTION((Get<Rigidbody, float, &Rigidbody::GetLinearDamping>)));
		r.Method("void set_linearDamping(float value) property", asFUNCTION((Set<Rigidbody, float, &Rigidbody::SetLinearDamping>)));
		r.Method("float get_angularDamping() const property", asFUNCTION((Get<Rigidbody, float, &Rigidbody::GetAngularDamping>)));
		r.Method("void set_angularDamping(float value) property", asFUNCTION((Set<Rigidbody, float, &Rigidbody::SetAngularDamping>)));
		// Unity 6 は linearVelocity。古い名前の velocity でも書けるようにする
		r.Method("Vector3 get_linearVelocity() const property", asFUNCTION((Get<Rigidbody, Vector3, &Rigidbody::GetLinearVelocity>)));
		r.Method("void set_linearVelocity(const Vector3 &in value) property", asFUNCTION((Set<Rigidbody, const Vector3 &, &Rigidbody::SetLinearVelocity>)));
		r.Method("Vector3 get_velocity() const property", asFUNCTION((Get<Rigidbody, Vector3, &Rigidbody::GetLinearVelocity>)));
		r.Method("void set_velocity(const Vector3 &in value) property", asFUNCTION((Set<Rigidbody, const Vector3 &, &Rigidbody::SetLinearVelocity>)));
		r.Method("Vector3 get_angularVelocity() const property", asFUNCTION((Get<Rigidbody, Vector3, &Rigidbody::GetAngularVelocity>)));
		r.Method("void set_angularVelocity(const Vector3 &in value) property", asFUNCTION((Set<Rigidbody, const Vector3 &, &Rigidbody::SetAngularVelocity>)));
		r.Method("Vector3 get_position() const property", asFUNCTION((Get<Rigidbody, Vector3, &Rigidbody::GetPosition>)));
		r.Method("Quaternion get_rotation() const property", asFUNCTION((Get<Rigidbody, Quaternion, &Rigidbody::GetRotation>)));
		r.Method("Vector3 get_worldCenterOfMass() const property", asFUNCTION((Get<Rigidbody, Vector3, &Rigidbody::GetWorldCenterOfMass>)));
		r.Method("void AddForce(const Vector3 &in force, ForceMode mode = ForceMode::Force)", asFUNCTION(AddForceWithMode<&Rigidbody::AddForce>));
		r.Method("void AddTorque(const Vector3 &in torque, ForceMode mode = ForceMode::Force)", asFUNCTION(AddForceWithMode<&Rigidbody::AddTorque>));
		r.Method("void AddRelativeForce(const Vector3 &in force, ForceMode mode = ForceMode::Force)", asFUNCTION(AddForceWithMode<&Rigidbody::AddRelativeForce>));
		r.Method("void AddRelativeTorque(const Vector3 &in torque, ForceMode mode = ForceMode::Force)", asFUNCTION(AddForceWithMode<&Rigidbody::AddRelativeTorque>));
		r.Method("void AddForceAtPosition(const Vector3 &in force, const Vector3 &in position, ForceMode mode = ForceMode::Force)", asFUNCTION(AddForceAtPosition));
		r.Method("void MovePosition(const Vector3 &in position)", asFUNCTION((Set<Rigidbody, const Vector3 &, &Rigidbody::MovePosition>)));
		r.Method("void MoveRotation(const Quaternion &in rotation)", asFUNCTION((Set<Rigidbody, const Quaternion &, &Rigidbody::MoveRotation>)));
		r.Method("void Sleep()", asFUNCTION(CallRigidbody<&Rigidbody::Sleep>));
		r.Method("void WakeUp()", asFUNCTION(CallRigidbody<&Rigidbody::WakeUp>));
		r.Method("bool IsSleeping() const", asFUNCTION((Get<Rigidbody, bool, &Rigidbody::IsSleeping>)));
	}

	// ---- Physics ----
	engine->SetDefaultNamespace("Physics");
	Check(engine->RegisterGlobalProperty("const int DefaultRaycastLayers", const_cast<int *>(&kDefaultRaycastLayers)), "Physics::DefaultRaycastLayers");
	Check(engine->RegisterGlobalProperty("const int AllLayers", const_cast<int *>(&kAllLayers)), "Physics::AllLayers");
	const auto global = [&](const char *declaration, const asSFuncPtr &function) {
		Check(engine->RegisterGlobalFunction(declaration, function, asCALL_CDECL), declaration);
	};
	global("Vector3 get_gravity() property", asFUNCTION(Physics::GetGravity));
	global("void set_gravity(const Vector3 &in value) property", asFUNCTION(Physics::SetGravity));
	global("bool Raycast(const Vector3 &in origin, const Vector3 &in direction, float maxDistance = Mathf::Infinity, int layerMask = Physics::DefaultRaycastLayers)", asFUNCTION(Raycast));
	global("bool Raycast(const Vector3 &in origin, const Vector3 &in direction, RaycastHit &out hitInfo, float maxDistance = Mathf::Infinity, int layerMask = Physics::DefaultRaycastLayers)", asFUNCTION(RaycastHitInfo));
	engine->SetDefaultNamespace("");
}
