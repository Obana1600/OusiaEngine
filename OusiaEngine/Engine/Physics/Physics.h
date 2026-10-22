#pragma once

#include <vector>


namespace ou {

// 前方宣言
class Component;
class Collider;
class BoxCollider;
class SphereCollider;

class Physics final {
	friend class Collider;

public: // 静的メンバ関数

	static void Simulate();


public: // インナークラス

	struct ContactPair {
		Collider *a = nullptr;
		Collider *b = nullptr;

		bool operator==(const ContactPair &) const = default;
	};


private: // 静的メンバ変数

	static inline std::vector<Collider *> sColliders_;
	static inline std::vector<ContactPair> sContacts_;


private: // 静的メンバ関数

	static void RegisterCollider(Collider *collider);
	static void UnregisterCollider(Collider *collider);

	using Message = void (Component::*)(Collider *);
	static void SendTriggerMessage(const ContactPair &pair, Message message);

	static bool Intersects(const Collider *a, const Collider *b);
	static bool IntersectsBoxBox(const BoxCollider *a, const BoxCollider *b);
	static bool IntersectsSphereSphere(const SphereCollider *a, const SphereCollider *b);
	static bool IntersectsSphere(const SphereCollider *a, const Collider *b);
};

} // namespace ou
