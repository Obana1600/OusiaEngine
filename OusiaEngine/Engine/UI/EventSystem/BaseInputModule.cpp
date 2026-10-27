#include "BaseInputModule.h"
#include "Scene/GameObject.h"
#include "Scene/Transform.h"
#include "UI/EventSystem/EventInterfaces.h"
#include "UI/EventSystem/EventSystem.h"
#include "UI/EventSystem/ExecuteEvents.h"

using namespace ou;


namespace {

GameObject *FindParentGameObject(const GameObject *gameObject) {
	auto *parent = gameObject->GetTransform()->GetParent();
	return parent ? parent->GetGameObject() : nullptr;
}

} // namespace



bool ou::BaseInputModule::IsPointerOverGameObject() const {
	return false;
}


GameObject *ou::BaseInputModule::FindCommonRoot(GameObject *gameObject1, GameObject *gameObject2) {
	for (auto *t1 = gameObject1; t1; t1 = FindParentGameObject(t1)) {
		for (auto *t2 = gameObject2; t2; t2 = FindParentGameObject(t2)) {
			if (t1 == t2) { return t1; }
		}
	}
	return nullptr;
}


EventSystem *ou::BaseInputModule::GetEventSystem() const {
	return GetGameObject()->GetComponent<EventSystem>();
}


void ou::BaseInputModule::HandlePointerExitAndEnter(PointerEventData &currentPointerData, GameObject *newEnterTarget) {
	if (currentPointerData.pointerEnter == newEnterTarget) { return; }

	auto *commonRoot = FindCommonRoot(currentPointerData.pointerEnter, newEnterTarget);

	for (auto *t = currentPointerData.pointerEnter; t && t != commonRoot; t = FindParentGameObject(t)) {
		ExecuteEvents::Execute(t, currentPointerData, &IPointerExitHandler::OnPointerExit);
	}

	currentPointerData.pointerEnter = newEnterTarget;

	for (auto *t = newEnterTarget; t && t != commonRoot; t = FindParentGameObject(t)) {
		ExecuteEvents::Execute(t, currentPointerData, &IPointerEnterHandler::OnPointerEnter);
	}
}
