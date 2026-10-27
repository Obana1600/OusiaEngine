#pragma once

#include "UI/EventSystem/EventData.h"


namespace ou {

class IEventSystemHandler {
public: // メンバ関数

	virtual ~IEventSystemHandler() = default;
};


class IPointerEnterHandler: public IEventSystemHandler {
public: // メンバ関数

	virtual void OnPointerEnter(PointerEventData &eventData) = 0;
};

class IPointerExitHandler:public IEventSystemHandler {
public: // メンバ関数

	virtual void OnPointerExit(PointerEventData &eventData) = 0;
};

class IPointerDownHandler:public IEventSystemHandler {
public: // メンバ関数

	virtual void OnPointerDown(PointerEventData &eventData) = 0;
};

class IPointerUpHandler: public IEventSystemHandler {
public: // メンバ関数

	virtual void OnPointerUp(PointerEventData &eventData) = 0;
};

class IPointerClickHandler: public IEventSystemHandler {
public: // メンバ関数

	virtual void OnPointerClick(PointerEventData &eventData) = 0;
};


class ISelectHandler: public IEventSystemHandler {
public: // メンバ関数

	virtual void OnSelect(BaseEventData &eventData) = 0;
};

class IDeselectHandler: public IEventSystemHandler {
public: // メンバ関数

	virtual void OnDeselect(BaseEventData &eventData) = 0;
};

class IMoveHandler: public IEventSystemHandler {
public: // メンバ関数

	virtual void OnMove(AxisEventData &eventData) = 0;
};

class ISubmitHandler: public IEventSystemHandler {
public: // メンバ関数

	virtual void OnSubmit(BaseEventData &eventData) = 0;
};

} // namespace ou
