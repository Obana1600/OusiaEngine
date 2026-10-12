#pragma once

#include "Object.h"
#include "Scene/Component.h"

#include <memory>
#include <string_view>
#include <type_traits>
#include <utility>
#include <vector>


namespace ou {

// 前方宣言
class Scene;
class Transform;

class GameObject final: public Object {
	friend class Scene;

public: // メンバ関数

	GameObject(Scene *scene, std::string_view name);
	~GameObject() override;

	template <typename T, typename... Args>
	T *AddComponent(Args &&...args);

	template <typename T>
	T *GetComponent() const;

	Transform *GetTransform() const;
	Scene *GetScene() const;


private: // メンバ変数

	Transform *transform_ = nullptr;
	std::vector<std::unique_ptr<Component>> components_;

	Scene *scene_ = nullptr;


private: // メンバ関数

	void StartComponents();
	void UpdateComponents();
	void LateUpdateComponents();

	void DestroyMarkedComponents();

	void NotifyDestroyToAllComponents();
};



template <typename T, typename... Args>
inline T *GameObject::AddComponent(Args &&...args) {
	static_assert(std::is_base_of_v<Component, T>, "T は Component の派生クラスでなければいけません。");
	static_assert(!std::is_same_v<T, Transform>, "Transform は GameObject が最初から持っているため、新たに追加することはできません。");

	auto component = std::make_unique<T>(std::forward<Args>(args)...);
	T *ptr = component.get();
	ptr->gameObject_ = this;
	ptr->transform_ = transform_;
	components_.push_back(std::move(component));

	ptr->Awake();
	return ptr;
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
