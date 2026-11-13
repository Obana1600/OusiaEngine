#pragma once

#include "Editor/EditorWindow.h"


namespace ou {

// 前方宣言
class Component;
class GameObject;

class InspectorWindow final: public EditorWindow {
public: // メンバ関数

	InspectorWindow();


protected: // メンバ関数

	void OnGui() override;


private: // メンバ変数

	void DrawGameObjectHeader(GameObject *gameObject);
	void DrawComponent(Component *component);
	void DrawComponentMenu(Component *component);
	void DrawAddComponentButton(GameObject *gameObject);
};

} // namespace ou
