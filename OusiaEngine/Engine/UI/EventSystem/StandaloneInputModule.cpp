#include "StandaloneInputModule.h"
#include "Input/Input.h"
#include "Platform/Screen.h"
#include "Serialize/Serializer.h"
#include "UI/EventSystem/EventInterfaces.h"
#include "UI/EventSystem/EventSystem.h"
#include "UI/EventSystem/ExecuteEvents.h"

#include <utility>

using namespace ou;


// TODO: どこかのファイルに分離したい
template <>
struct magic_enum::customize::enum_range<ou::KeyCode> {
	static constexpr int min = 0;
	static constexpr int max = 511;
};



void ou::StandaloneInputModule::Serialize(Serializer &serializer) {
	BaseInputModule::Serialize(serializer);

	const auto key = [&](KeyCode &value) { serializer.Enum("", value); };
	serializer.Array("upKeys", upKeys_, key);
	serializer.Array("downKeys", downKeys_, key);
	serializer.Array("submitKeys", submitKeys_, key);
}


void ou::StandaloneInputModule::Process() {
	if (!SendMoveEventToSelectedObject()) {
		SendSubmitEventToSelectedObject();
	}

	ProcessMouseEvent();
}


bool ou::StandaloneInputModule::IsPointerOverGameObject() const {
	return pointerData_.pointerEnter != nullptr;
}


void ou::StandaloneInputModule::OnGameObjectDestroyed(const GameObject *gameObject) {
	if (pointerData_.pointerEnter == gameObject) {
		pointerData_.pointerEnter = nullptr;
	}

	if (pointerData_.pointerPress == gameObject || pointerData_.rawPointerPress == gameObject) {
		pointerData_.pointerPress = nullptr;
		pointerData_.rawPointerPress = nullptr;
		pointerData_.eligibleForClick = false;
	}

	if (pointerData_.pointerCurrentRaycast.gameObject == gameObject) {
		pointerData_.pointerCurrentRaycast = {};
	}
}


void ou::StandaloneInputModule::SetUpKeys(std::vector<KeyCode> keys) {
	upKeys_ = std::move(keys);
}


const std::vector<KeyCode> &ou::StandaloneInputModule::GetUpKeys() const {
	return upKeys_;
}


void ou::StandaloneInputModule::SetDownKeys(std::vector<KeyCode> keys) {
	downKeys_ = std::move(keys);
}


const std::vector<KeyCode> &ou::StandaloneInputModule::GetDownKeys() const {
	return downKeys_;
}


void ou::StandaloneInputModule::SetSubmitKeys(std::vector<KeyCode> keys) {
	submitKeys_ = std::move(keys);
}


const std::vector<KeyCode> &ou::StandaloneInputModule::GetSubmitKeys() const {
	return submitKeys_;
}


bool ou::StandaloneInputModule::IsAnyKeyDown(const std::vector<KeyCode> &keys) {
	for (const auto key : keys) {
		if (Input::GetKeyDown(key)) { return true; }
	}
	return false;
}


bool ou::StandaloneInputModule::SendMoveEventToSelectedObject() {
	AxisEventData axisEventData;
	if (IsAnyKeyDown(upKeys_)) {
		axisEventData.moveDir = MoveDirection::Up;
	} else if (IsAnyKeyDown(downKeys_)) {
		axisEventData.moveDir = MoveDirection::Down;
	} else {
		return false;
	}

	auto *selected = GetEventSystem()->GetCurrentSelectedGameObject();
	return ExecuteEvents::Execute(selected, axisEventData, &IMoveHandler::OnMove);
}


bool ou::StandaloneInputModule::SendSubmitEventToSelectedObject() {
	if (!IsAnyKeyDown(submitKeys_)) { return false; }

	auto *selected = GetEventSystem()->GetCurrentSelectedGameObject();
	BaseEventData eventData;
	return ExecuteEvents::Execute(selected, eventData, &ISubmitHandler::OnSubmit);
}


void ou::StandaloneInputModule::ProcessMouseEvent() {
	const auto mouse = Input::GetMousePosition();
	pointerData_.position = {mouse.x,
							 static_cast<float>(Screen::GetHeight()) - mouse.y};
	pointerData_.button = PointerEventData::InputButton::Left;

	std::vector<RaycastResult> raycastResults;
	GetEventSystem()->RaycastAll(pointerData_, raycastResults);
	pointerData_.pointerCurrentRaycast = raycastResults.empty() ? RaycastResult{}
																: raycastResults.front();

	auto *currentOverGo = pointerData_.pointerCurrentRaycast.gameObject;
	ProcessMousePress(currentOverGo);
	HandlePointerExitAndEnter(pointerData_, currentOverGo);
}


void ou::StandaloneInputModule::ProcessMousePress(GameObject *currentOverGo) {
	if (Input::GetMouseButtonDown(0)) {
		pointerData_.eligibleForClick = true;

		auto *newPressed = ExecuteEvents::ExecuteHierarchy(currentOverGo, pointerData_, &IPointerDownHandler::OnPointerDown);
		if (!newPressed) {
			newPressed = ExecuteEvents::GetEventHandler<IPointerClickHandler>(currentOverGo);
		}

		pointerData_.pointerPress = newPressed;
		pointerData_.rawPointerPress = currentOverGo;
	}

	if (Input::GetMouseButtonUp(0)) {
		ExecuteEvents::Execute(pointerData_.pointerPress, pointerData_, &IPointerUpHandler::OnPointerUp);

		auto *pointerupHandler = ExecuteEvents::GetEventHandler<IPointerClickHandler>(currentOverGo);
		if (pointerData_.pointerPress && pointerData_.pointerPress == pointerupHandler && pointerData_.eligibleForClick) {
			ExecuteEvents::Execute(pointerData_.pointerPress, pointerData_, &IPointerClickHandler::OnPointerClick);
		}

		pointerData_.eligibleForClick = false;
		pointerData_.pointerPress = nullptr;
		pointerData_.rawPointerPress = nullptr;
	}
}
