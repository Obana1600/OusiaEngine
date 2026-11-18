#pragma once

#include "Math/Mathf.h"
#include "Math/Quaternion.h"
#include "Math/Vector3.h"
#include "Physics/Box3D/PhysicsWorld.h"
#include "Physics/RaycastHit.h"
#include "Scene/LayerMask.h"

#include <array>
#include <cstdint>
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
	friend class CharacterController;

public: // 定数

	static constexpr uint32_t kIgnoreRaycastLayer = 1u << 1;
	static constexpr uint32_t kDefaultRaycastLayers = ~kIgnoreRaycastLayer;
	static constexpr uint32_t kAllLayers = ~0u;


public: // 静的メンバ関数

	static void Simulate();

	static void Interpolate();

	static void SyncTransform();

	static void SetGravity(const Vector3 &gravity);
	static Vector3 GetGravity();

	static void SetQueriesHitTriggers(bool value);
	static bool GetQueriesHitTriggers();

	static void IgnoreLayerCollision(int layer1, int layer2, bool ignore = true);
	static bool GetIgnoreLayerCollision(int layer1, int layer2);

	// ProjectSettings/DynamicsManager.json を読む（読む前に既定の値に戻す。Play を止めたときにも読み直す）
	static void LoadDynamicsManager();
	// 今の重力・queriesHitTriggers・当たらないレイヤーの組を書く（組はレイヤーの名前で書くので、名前のないレイヤーの組は書かない）
	static void SaveDynamicsManager();


	static bool Raycast(
		const Vector3 &origin, const Vector3 &direction,
		RaycastHit *hitInfo = nullptr,
		float maxDistance = Mathf::Infinity,
		uint32_t layerMask = kDefaultRaycastLayers,
		QueryTriggerInteracation queryTriggerInteraction = QueryTriggerInteracation::UseGlobal);

	static std::vector<RaycastHit> RaycastAll(
		const Vector3 &origin, const Vector3 &direction,
		float maxDistance = Mathf::Infinity,
		uint32_t layerMask = kDefaultRaycastLayers,
		QueryTriggerInteracation queryTriggerInteraction = QueryTriggerInteracation::UseGlobal);


	static std::vector<Collider *> OverlapBox(
		const Vector3 &center, const Vector3 &halfExtents,
		const Quaternion &orientation = Quaternion::Identity,
		uint32_t layerMask = kAllLayers,
		QueryTriggerInteracation queryTriggerInteraction = QueryTriggerInteracation::UseGlobal);

	static std::vector<Collider *> OverlapSphere(
		const Vector3 &center, float radius,
		uint32_t layerMask = kAllLayers,
		QueryTriggerInteracation queryTriggerInteraction = QueryTriggerInteracation::UseGlobal);

	static std::vector<Collider *> OverlapCapsule(
		const Vector3 &point0, const Vector3 &point1, float radius,
		uint32_t layerMask = kAllLayers,
		QueryTriggerInteracation queryTriggerInteraction = QueryTriggerInteracation::UseGlobal);


	static bool BoxCast(
		const Vector3 &center, const Vector3 &halfExtents, const Vector3 &direction,
		const Quaternion &orientation = Quaternion::Identity,
		RaycastHit *hitInfo = nullptr,
		float maxDistance = Mathf::Infinity,
		uint32_t layerMask = kDefaultRaycastLayers,
		QueryTriggerInteracation queryTriggerInteraction = QueryTriggerInteracation::UseGlobal);

	static std::vector<RaycastHit> BoxCastAll(
		const Vector3 &center, const Vector3 &halfExtents, const Vector3 &direction,
		const Quaternion &orientation = Quaternion::Identity,
		float maxDistance = Mathf::Infinity,
		uint32_t layerMask = kDefaultRaycastLayers,
		QueryTriggerInteracation queryTriggerInteraction = QueryTriggerInteracation::UseGlobal);

	static bool SphereCast(
		const Vector3 &origin, float radius,
		const Vector3 &direction,
		RaycastHit *hitInfo = nullptr,
		float maxDistance = Mathf::Infinity,
		uint32_t layerMask = kDefaultRaycastLayers,
		QueryTriggerInteracation queryTriggerInteraction = QueryTriggerInteracation::UseGlobal);

	static std::vector<RaycastHit> SphereCastAll(
		const Vector3 &origin, float radius,
		const Vector3 &direction,
		float maxDistance = Mathf::Infinity,
		uint32_t layerMask = kDefaultRaycastLayers,
		QueryTriggerInteracation queryTriggerInteraction = QueryTriggerInteracation::UseGlobal);

	static bool CapsuleCast(
		const Vector3 &point1, const Vector3 &point2, float radius,
		const Vector3 &direction,
		RaycastHit *hitInfo = nullptr,
		float maxDistance = Mathf::Infinity,
		uint32_t layerMask = kDefaultRaycastLayers,
		QueryTriggerInteracation queryTriggerInteraction = QueryTriggerInteracation::UseGlobal);

	static std::vector<RaycastHit> CapsuleCastAll(
		const Vector3 &point1, const Vector3 &point2, float radius,
		const Vector3 &direction,
		float maxDistance = Mathf::Infinity,
		uint32_t layerMask = kDefaultRaycastLayers,
		QueryTriggerInteracation queryTriggerInteraction = QueryTriggerInteracation::UseGlobal);


	static bool CheckBox(
		const Vector3 &center, const Vector3 &halfExtents,
		const Quaternion &orientation = Quaternion::Identity,
		uint32_t layerMask = kDefaultRaycastLayers,
		QueryTriggerInteracation queryTriggerInteraction = QueryTriggerInteracation::UseGlobal);

	static bool CheckSphere(
		const Vector3 &position, float radius,
		uint32_t layerMask = kDefaultRaycastLayers,
		QueryTriggerInteracation queryTriggerInteraction = QueryTriggerInteracation::UseGlobal);

	static bool CheckCapsule(
		const Vector3 &start, const Vector3 &end, float radius,
		uint32_t layerMask = kDefaultRaycastLayers,
		QueryTriggerInteracation queryTriggerInteraction = QueryTriggerInteracation::UseGlobal);


private: // 静的メンバ変数

	static inline std::vector<Collider *> sColliders_;
	static inline std::vector<Collider *> sPendingColliders_;
	static inline std::vector<Rigidbody *> sRigidbodies_;
	static inline bool sQueryHitTriggers_ = true;
	static inline std::array<uint32_t, LayerMask::kLayerCount> sIgnoreLayerMasks_{};


private: // 静的メンバ関数

	static void RegisterCollider(Collider *collider);
	static void UnregisterCollider(Collider *collider);

	static void SyncPendingColliders();

	static void RegisterRigidbody(Rigidbody *rigidbody);
	static void UnregisterRigidbody(Rigidbody *rigidbody);

	static void DispatchContactEvents();
	static void SendToReceivers(Collider *collider, const std::function<void(Component *)> &send);

	static bool HitsTriggers(QueryTriggerInteracation queryTriggerInteraction);
	static Collider *FindCollider(PhysicsWorld::ShapeId id);
	static std::vector<Collider *> FindColliders(const std::vector<PhysicsWorld::ShapeId> &ids);

	static std::vector<RaycastHit> ToCastHits(const std::vector<PhysicsWorld::RaycastHit> &results, const Vector3 &direction);
	static bool FindFirstCastHit(const std::vector<RaycastHit> &hits, RaycastHit *hitInfo);

	static PhysicsWorld::CollisionFilter CalculateCollisionFilter(int layer);
};

} // namespace ou
