#pragma once

#include "Input/KeyCode.h"
#include "UI/EventSystem/BaseInputModule.h"

#include <vector>


namespace ou {

class StandaloneInputModule final: public BaseInputModule {
public: // メンバ関数

	void Serialize(Serializer &serializer) override;

	void Process() override;

	bool IsPointerOverGameObject() const override;

	void OnGameObjectDestroyed(const GameObject *gameObject) override;

	void SetUpKeys(std::vector<KeyCode> keys);
	const std::vector<KeyCode> &GetUpKeys() const;

	void SetDownKeys(std::vector<KeyCode> keys);
	const std::vector<KeyCode> &GetDownKeys() const;

	void SetSubmitKeys(std::vector<KeyCode> keys);
	const std::vector<KeyCode> &GetSubmitKeys() const;


private: // メンバ変数

	PointerEventData pointerData_;

	std::vector<KeyCode> upKeys_ = {KeyCode::W, KeyCode::UpArrow};
	std::vector<KeyCode> downKeys_ = {KeyCode::S, KeyCode::DownArrow};
	std::vector<KeyCode> submitKeys_ = {KeyCode::Space, KeyCode::Return};


private: // 静的メンバ関数

	static bool IsAnyKeyDown(const std::vector<KeyCode> &keys);


private: // メンバ関数

	bool SendMoveEventToSelectedObject();
	bool SendSubmitEventToSelectedObject();

	void ProcessMouseEvent();
	void ProcessMousePress(GameObject *currentOverGo);
};

} // namespace ou
