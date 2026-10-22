#include "Scene.h"
#include "Scene/Component.h"
#include "Scene/Transform.h"

#include <algorithm>

using namespace ou;


ou::Scene::Scene(std::string_view name, int32_t buildIndex)
	: name_(name),
	  buildIndex_(buildIndex) {}


ou::Scene::~Scene() {
	DestroyAllGameObjects();
}


GameObject *ou::Scene::CreateGameObject(std::string_view name) {
	auto obj = std::make_unique<GameObject>(this, name);
	gameObjects_.push_back(std::move(obj));
	return gameObjects_.back().get();
}


GameObject *ou::Scene::Find(std::string_view name) const {
	for (const auto &obj : gameObjects_) {
		if (!obj) { continue; }
		if (!obj->IsDestroyed() && obj->GetName() == name) {
			return obj.get();
		}
	}

	return nullptr;
}


const std::string &ou::Scene::GetName() const {
	return name_;
}


int32_t ou::Scene::GetBuildIndex() const {
	return buildIndex_;
}


void ou::Scene::StartGameObjects() {
	const auto count = gameObjects_.size();

	for (size_t i = 0; i < count; i++) {
		if (gameObjects_[i] && !gameObjects_[i]->IsDestroyed()) {
			gameObjects_[i]->StartComponents();
		}
	}
}


void ou::Scene::UpdateGameObjects() {
	const auto count = gameObjects_.size();

	for (size_t i = 0; i < count; i++) {
		if (gameObjects_[i] && !gameObjects_[i]->IsDestroyed()) {
			gameObjects_[i]->UpdateComponents();
		}
	}
}


void ou::Scene::LateUpdateGameObjects() {
	const auto count = gameObjects_.size();

	for (size_t i = 0; i < count; i++) {
		if (gameObjects_[i] && !gameObjects_[i]->IsDestroyed()) {
			gameObjects_[i]->LateUpdateComponents();
		}
	}
}


void ou::Scene::DestroyMarkedObjects() {
	for (const auto &obj : gameObjects_) {
		if (!obj) { continue; }
		if (obj->IsDestroyed()) {
			MarkDescendantsDestroyed(obj.get());
		}
	}

	for (const auto &obj : gameObjects_) {
		if (!obj) { continue; }
		if (obj->IsDestroyed()) {
			obj->NotifyDestroyToAllComponents();
		} else {
			obj->DestroyMarkedComponents();
		}
	}

	// 実際に消す
	std::erase_if(gameObjects_, [](const auto &obj) {
		return !obj || obj->IsDestroyed();
	});
}


void ou::Scene::DestroyAllGameObjects() {
	for (const auto &obj : gameObjects_) {
		if (!obj) { continue; }
		obj->NotifyDestroyToAllComponents();
	}
	gameObjects_.clear();
}


void ou::Scene::MarkDescendantsDestroyed(GameObject *gameObject) {
	for (auto *child : gameObject->GetTransform()->GetChildren()) {
		Object::Destroy(child->GetGameObject());
		MarkDescendantsDestroyed(child->GetGameObject());
	}
}
