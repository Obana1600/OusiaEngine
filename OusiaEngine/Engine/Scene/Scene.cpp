#include "Scene.h"
#include "Scene/Component.h"
#include "Scene/Transform.h"

#include <algorithm>

using namespace ou;


uint64_t ou::Scene::GetDestroyGeneration() {
	return sDestroyGeneration_;
}


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


std::vector<GameObject *> ou::Scene::GetRootGameObjects() const {
	std::vector<GameObject *> roots;

	// 親のない GameObject を集める
	for (const auto &gameObject : gameObjects_) {
		if (gameObject && !gameObject->GetTransform()->GetParent()) {
			roots.push_back(gameObject.get());
		}
	}

	return roots;
}


const std::string &ou::Scene::GetName() const {
	return name_;
}


const std::string &ou::Scene::GetPath() const {
	return path_;
}


int32_t ou::Scene::GetBuildIndex() const {
	return buildIndex_;
}


bool ou::Scene::IsDirty() const {
	return isDirty_;
}


void ou::Scene::StartGameObjects() {
	const auto count = gameObjects_.size();

	for (size_t i = 0; i < count; i++) {
		if (gameObjects_[i] && !gameObjects_[i]->IsDestroyed()) {
			gameObjects_[i]->StartComponents();
		}
	}
}


void ou::Scene::FixedUpdateGameObjects() {
	const auto count = gameObjects_.size();

	for (size_t i = 0; i < count; i++) {
		if (gameObjects_[i] && !gameObjects_[i]->IsDestroyed()) {
			gameObjects_[i]->FixedUpdateComponents();
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

	const auto erasedCount = std::erase_if(gameObjects_, [](const auto &obj) {
		return !obj || obj->IsDestroyed();
	});
	if (erasedCount > 0) { sDestroyGeneration_++; }
}


void ou::Scene::DestroyAllGameObjects() {
	for (const auto &obj : gameObjects_) {
		if (!obj) { continue; }
		obj->NotifyDestroyToAllComponents();
	}
	gameObjects_.clear();
	sDestroyGeneration_++;
}


void ou::Scene::MarkDescendantsDestroyed(GameObject *gameObject) {
	for (auto *child : gameObject->GetTransform()->GetChildren()) {
		Object::Destroy(child->GetGameObject());
		MarkDescendantsDestroyed(child->GetGameObject());
	}
}
