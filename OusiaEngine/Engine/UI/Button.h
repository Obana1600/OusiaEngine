#pragma once

#include "Core/Event.h"
#include "UI/Selectable.h"


namespace ou {

class Button: public Selectable,
			  public IPointerClickHandler,
			  public ISubmitHandler {
public: // メンバ変数

	Event<> onClick_;


public: // メンバ関数

	void Update() override;

	void OnPointerClick(PointerEventData &eventData) override;
	void OnSubmit(BaseEventData &evemtData) override;


private: // メンバ変数

	float finishSubmitTimer_ = 0.0f;


private: // メンバ関数

	void Press();
};

} // namespace ou
