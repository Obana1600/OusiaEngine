#pragma once

#include "Scene/Behaviour.h"
#include "Scene/GameObject.h"
#include "Scene/Transform.h"


namespace ou::ExecuteEvents {

template <typename T>
T *FindHandler(GameObject *gameObject);

template <typename T, typename Data>
bool Execute(GameObject *target, Data &eventData, void (T::*function)(Data &));

template <typename T>
GameObject *GetEventHandler(GameObject *root);

template <typename T, typename Data>
GameObject *ExecuteHierarchy(GameObject *root, Data &eventData, void (T::*function)(Data &));



template <typename T>
T *FindHandler(GameObject *gameObject) {
	if (!gameObject) { return nullptr; }

	auto *handler = gameObject->GetComponent<T>();
	const auto *behavior = dynamic_cast<const Behaviour *>(handler);
	if (behavior && !behavior->IsActiveAndEnabled()) { return nullptr; }
	return handler;
}

template <typename T, typename Data>
bool Execute(GameObject *target, Data &eventData, void (T::*function)(Data &)) {
	auto *handler = FindHandler<T>(target);
	if (!handler) { return false; }

	(handler->*function)(eventData);
	return true;
}

template <typename T>
GameObject *GetEventHandler(GameObject *root) {
	for (auto *gameObject = root; gameObject;) {
		if (FindHandler<T>(gameObject)) { return gameObject; }

		auto *parent = gameObject->GetTransform()->GetParent();
		gameObject = parent ? parent->GetGameObject() : nullptr;
	}
	return nullptr;
}

template <typename T, typename Data>
GameObject *ExecuteHierarchy(GameObject *root, Data &eventData, void (T::*function)(Data &)) {
	auto *handlerObject = GetEventHandler<T>(root);
	Execute(handlerObject, eventData, function);
	return handlerObject;
}

} // namespace ou::ExecuteEvents
