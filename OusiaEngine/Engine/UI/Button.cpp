#include "Button.h"
#include "Core/Time.h"

using namespace ou;


void ou::Button::Update() {
	if (finishSubmitTimer_ <= 0.0f) { return; }

	finishSubmitTimer_ -= Time::GetUnscaledDeltaTime();
	if (finishSubmitTimer_ <= 0.0f) {
		DoStateTransition(GetCurrentSelectionState(), false);
	}
}


void ou::Button::OnPointerClick(PointerEventData &eventData) {
	if (eventData.button != PointerEventData::InputButton::Left) { return; }
	Press();
}


void ou::Button::OnSubmit(BaseEventData &) {
	Press();

	if (!IsActiveAndEnabled() || !IsInteractable()) { false; }

	DoStateTransition(SelectionState::Pressed, false);
	finishSubmitTimer_ = GetColors().fadeDuration;
}


void ou::Button::Press() {
	if (!IsActiveAndEnabled() || !IsInteractable()) { return; }
	onClick_.Invoke();
}
