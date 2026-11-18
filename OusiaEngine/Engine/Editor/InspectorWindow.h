#pragma once

#include "Editor/Editor.h"
#include "Editor/EditorWindow.h"

#include <cstdint>
#include <memory>
#include <string>
#include <vector>


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

	std::string addComponentFilter_;

	uint32_t inspectedId_ = 0;
	std::vector<uint32_t> componentIds_;
	std::vector<std::unique_ptr<Editor>> editors_;


private: // メンバ関数

	void RebuildEditorsIfNeeded(GameObject *gameObject);

	void DrawGameObjectHeader(GameObject *gameObject);
	void DrawComponent(Editor *editor);
	void DrawComponentMenu(Component *component);
	void DrawAddComponentButton(GameObject *gameObject);
	void DrawAddScriptItems(GameObject *gameObject);
};

} // namespace ou
