#pragma once

#include "Scene/Behaviour.h"
#include "UI/EventSystem/EventData.h"


namespace ou {

// 前方宣言
class EventSystem;

class BaseInputModule: public Behaviour {
public: // メンバ関数

	virtual void Process() = 0;

	virtual bool IsPointerOverGameObject() const;

	virtual void OnGameObjectDestroyed(const GameObject *) {};


protected: // 静的メンバ関数

	static GameObject *FindCommonRoot(GameObject *gameObject1, GameObject *gameObject2);


protected: // メンバ関数

	EventSystem *GetEventSystem() const;

	void HandlePointerExitAndEnter(PointerEventData &currentPointerData, GameObject *newEnterTarget);
};

} // namespace ou
