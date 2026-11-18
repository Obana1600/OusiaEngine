#pragma once

#include "Events/UnityEvent.h"
#include "UI/Selectable.h"


namespace ou {

class Button: public Selectable,
			  public IPointerClickHandler,
			  public ISubmitHandler {
public: // メンバ変数

	// Inspector でつないだもの（シーンに保存）と、AddListener でつないだものを呼ぶ
	UnityEvent onClick_;


public: // メンバ関数

	void Update() override;
	void Serialize(Serializer &serializer) override;

	void OnPointerClick(PointerEventData &eventData) override;
	void OnSubmit(BaseEventData &evemtData) override;


private: // メンバ変数

	float finishSubmitTimer_ = 0.0f;


private: // メンバ関数

	void Press();
};

} // namespace ou
