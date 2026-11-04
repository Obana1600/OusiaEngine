#pragma once

#include "Core/Object.h"
#include "Scene/Component.h"
#include "Scene/Transform.h"

#include <memory>
#include <string_view>
#include <type_traits>
#include <utility>
#include <vector>


namespace ou {

// 前方宣言
class Scene;

class GameObject final: public Object {
	friend class Scene;
	friend class SceneManager;
	friend class Transform;
	friend class Physics;

public: // メンバ関数

	GameObject(Scene *scene, std::string_view name);
	~GameObject() override;

	template <typename T, typename... Args>
	T *AddComponent(Args &&...args);

	template <typename T>
	T *GetComponent() const;

	Transform *GetTransform() const;
	Scene *GetScene() const;

	void SetActive(bool value);
	bool IsActiveSelf() const;
	bool IsActiveInHierarchy() const;

	void SetLayer(int layer);
	int GetLayer() const;


private: // メンバ変数

	std::vector<std::unique_ptr<Component>> components_;

	bool isActiveSelf_ = true;
	int layer_ = 0;

	Transform *transform_ = nullptr;
	Scene *scene_ = nullptr;


private: // メンバ関数

	void AwakeComponents();
	void StartComponents();
	void FixedUpdateComponents();
	void UpdateComponents();
	void LateUpdateComponents();

	void DestroyMarkedComponents();
	void NotifyDestroyToAllComponents();

	void ReplaceTransform(std::unique_ptr<Transform> newTransform);

	void RefreshActiveInHierarchy();

	static void RefreshBehavior(Component *component);
	static void DisableBeheviorForDestroy(Component *component);
};



template <typename T, typename... Args>
inline T *GameObject::AddComponent(Args &&...args) {
	static_assert(std::is_base_of_v<Component, T>, "T は Component の派生クラスでなければいけません。");
	static_assert(!std::is_same_v<T, Transform>, "Transform は GameObject が最初から持っているため、新たに追加することはできません。");

	// T が Transform の派生の場合、今の Transform と差し替える
	if constexpr (std::is_base_of_v<Transform, T>) {
		auto transform = std::make_unique<T>(std::forward<Args>(args)...);
		T *ptr = transform.get();
		ReplaceTransform(std::move(transform));
		return ptr;

	} else {
		auto component = std::make_unique<T>(std::forward<Args>(args)...);
		T *ptr = component.get();
		ptr->gameObject_ = this;
		ptr->transform_ = transform_;
		components_.push_back(std::move(component));

		if (IsActiveInHierarchy()) {
			ptr->isAwoken_ = true;
			ptr->Awake();
			RefreshBehavior(ptr);
		}
		return ptr;
	}
}


template <typename T>
inline T *GameObject::GetComponent() const {
	for (const auto &component : components_) {
		if (auto *ptr = dynamic_cast<T *>(component.get())) {
			return ptr;
		}
	}

	return nullptr;
}

} // namespace ou
