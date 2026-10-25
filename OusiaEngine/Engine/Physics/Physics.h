#pragma once

#include <functional>
#include <vector>


namespace ou {

// 前方宣言
class Component;
class Collider;
class Rigidbody;

class Physics final {
	friend class Collider;
	friend class Rigidbody;

public: // 静的メンバ関数

	static void Simulate();

	static void SyncTransform();


private: // 静的メンバ変数

	static inline std::vector<Collider *> sColliders_;
	static inline std::vector<Rigidbody *> sRigidbodies_;


private: // 静的メンバ関数

	static void RegisterCollider(Collider *collider);
	static void UnregisterCollider(Collider *collider);

	static void RegisterRigidbody(Rigidbody *rigidbody);
	static void UnregisterRigidbody(Rigidbody *rigidbody);

	static void DispatchContactEvents();
	static void SendToReceivers(Collider *collider, const std::function<void(Component *)> &send);
};

} // namespace ou
