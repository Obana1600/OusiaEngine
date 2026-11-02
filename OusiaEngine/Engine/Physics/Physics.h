#pragma once

#include "Math/Mathf.h"
#include "Math/Quaternion.h"
#include "Math/Vector3.h"
#include "Physics/Box3D/PhysicsWorld.h"
#include "Physics/RaycastHit.h"

#include <functional>
#include <vector>


namespace ou {

// 前方宣言
class Component;
class Collider;
class Rigidbody;

enum class QueryTriggerInteracation {
	UseGlobal,
	Ignore,
	Collide,
};

class Physics final {
	friend class Collider;
	friend class Rigidbody;

public: // 静的メンバ関数

	static void Simulate();

	static void SyncTransform();

	static void SetGravity(const Vector3 &gravity);
	static Vector3 GetGravity();

	static void SetQueriesHitTriggers(bool value);
	static bool GetQueriesHitTriggers();

	static bool Raycast(const Vector3 &origin, const Vector3 &direction, RaycastHit *hitInfo = nullptr,
						float maxDistance = Mathf::Infinity, QueryTriggerInteracation queryTriggerInteraction = QueryTriggerInteracation::UseGlobal);
	static std::vector<RaycastHit> RaycastAll(const Vector3 &origin, const Vector3 &direction,
											  float maxDistance = Mathf::Infinity, QueryTriggerInteracation queryTriggerInteraction = QueryTriggerInteracation::UseGlobal);
	static std::vector<Collider *> OverlapBox(const Vector3 &center, const Vector3 &halfExtents, const Quaternion &orientation = Quaternion::Identity,
											  QueryTriggerInteracation queryTriggerInteraction = QueryTriggerInteracation::UseGlobal);
	static std::vector<Collider *> OverlapSphere(const Vector3 &center, float radius,
												 QueryTriggerInteracation queryTriggerInteraction = QueryTriggerInteracation::UseGlobal);
	static std::vector<Collider *> OverlapCapsule(const Vector3 &point0, const Vector3 &point1, float radius,
												  QueryTriggerInteracation queryTriggerInteraction = QueryTriggerInteracation::UseGlobal);


private: // 静的メンバ変数

	static inline std::vector<Collider *> sColliders_;
	static inline std::vector<Rigidbody *> sRigidbodies_;
	static inline bool sQueryHitTriggers_ = true;


private: // 静的メンバ関数

	static void RegisterCollider(Collider *collider);
	static void UnregisterCollider(Collider *collider);

	static void RegisterRigidbody(Rigidbody *rigidbody);
	static void UnregisterRigidbody(Rigidbody *rigidbody);

	static void DispatchContactEvents();
	static void SendToReceivers(Collider *collider, const std::function<void(Component *)> &send);

	static bool HitsTriggers(QueryTriggerInteracation queryTriggerInteraction);
	static Collider *FindCollider(PhysicsWorld::ShapeId id);
	static std::vector<Collider *> FindColliders(const std::vector<PhysicsWorld::ShapeId> &ids);
};

} // namespace ou
