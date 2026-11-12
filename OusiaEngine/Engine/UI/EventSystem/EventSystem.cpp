#include "EventSystem.h"
#include "Diagnostics/Log.h"
#include "Scene/GameObject.h"
#include "Serialize/Serializer.h"
#include "UI/EventSystem/BaseInputModule.h"
#include "UI/EventSystem/EventInterfaces.h"
#include "UI/EventSystem/ExecuteEvents.h"
#include "UI/GraphicRaycaster.h"

#include <algorithm>

using namespace ou;


EventSystem *ou::EventSystem::GetCurrent() {
	return sEventSystems_.empty() ? nullptr : sEventSystems_.front();
}


void ou::EventSystem::Start() {
	if (firstSelected_) {
		SetSelectedGameObject(firstSelected_);
	}
}


void ou::EventSystem::OnEnable() {
	sEventSystems_.push_back(this);
}


void ou::EventSystem::OnDisable() {
	std::erase(sEventSystems_, this);
}


void ou::EventSystem::Serialize(Serializer &serializer) {
	Behavior::Serialize(serializer);

	serializer.Reference("firstSelected", firstSelected_);
}


void ou::EventSystem::Process() {
	if (auto *module = FindInputModule()) {
		module->Process();
	}
}


void ou::EventSystem::SetFirstSelectedGameObject(GameObject *gameObject) {
	firstSelected_ = gameObject;
}


GameObject *ou::EventSystem::GetFirstSelectedGameObject() const {
	return firstSelected_;
}


void ou::EventSystem::SetSelectedGameObject(GameObject *selected) {
	if (isSelecting_) {
		Log::Error("選択中に SetSelectedGameObject は呼べません");
		return;
	}
	if (selected == currentSelected_) { return; }

	isSelecting_ = true;

	BaseEventData eventData;
	ExecuteEvents::Execute(currentSelected_, eventData, &IDeselectHandler::OnDeselect);
	currentSelected_ = selected;
	ExecuteEvents::Execute(currentSelected_, eventData, &ISelectHandler::OnSelect);

	isSelecting_ = false;
}


GameObject *ou::EventSystem::GetCurrentSelectedGameObject() const {
	return currentSelected_;
}


bool ou::EventSystem::IsPointerOverGameObject() const {
	const auto *module = FindInputModule();
	return module && module->IsPointerOverGameObject();
}


void ou::EventSystem::RaycastAll(const PointerEventData &eventData, std::vector<RaycastResult> &raycastResults) const {
	raycastResults.clear();


	const auto &raycasters = GraphicRaycaster::GetRaycasters();
	for (auto it = raycasters.rbegin(); it != raycasters.rend(); it++) {
		(*it)->Raycast(eventData, raycastResults);
	}
}


void ou::EventSystem::OnGameObjectDestroyed(const GameObject *gameObject) {
	if (currentSelected_ == gameObject) { currentSelected_ = nullptr; }
	if (firstSelected_ == gameObject) { firstSelected_ = nullptr; }

	if (auto *module = FindInputModule()) {
		module->OnGameObjectDestroyed(gameObject);
	}
}


BaseInputModule *ou::EventSystem::FindInputModule() const {
	auto *module = GetGameObject()->GetComponent<BaseInputModule>();
	return (module && module->IsActiveAndEnabled()) ? module : nullptr;
}
