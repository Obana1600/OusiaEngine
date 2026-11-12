#pragma once

#include "Scene/Behavior.h"
#include "UI/EventSystem/EventData.h"

#include <vector>


namespace ou {

// 前方宣言
class BaseInputModule;

class EventSystem final: public Behavior {
public: // 静的メンバ関数

	static EventSystem *GetCurrent();


public: // メンバ関数

	void Start() override;
	void OnEnable() override;
	void OnDisable() override;
	void Serialize(Serializer &serializer) override;

	void Process();

	void SetFirstSelectedGameObject(GameObject *gameObject);
	GameObject *GetFirstSelectedGameObject() const;

	void SetSelectedGameObject(GameObject *selected);
	GameObject *GetCurrentSelectedGameObject() const;

	bool IsPointerOverGameObject() const;

	void RaycastAll(const PointerEventData &eventData, std::vector<RaycastResult> &raycastResults) const;

	void OnGameObjectDestroyed(const GameObject *gameObject);


private: // メンバ変数

	GameObject *firstSelected_ = nullptr;
	GameObject *currentSelected_ = nullptr;

	bool isSelecting_ = false;


private: // 静的メンバ変数

	static inline std::vector<EventSystem *> sEventSystems_;


private: // メンバ関数

	BaseInputModule *FindInputModule() const;
};

} // namespace ou
