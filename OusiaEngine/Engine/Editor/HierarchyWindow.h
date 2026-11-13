#pragma once

#include "Editor/EditorWindow.h"


namespace ou {

// 前方宣言
class GameObject;
class Scene;

class HierarchyWindow final: public EditorWindow {
public: // メンバ関数

	HierarchyWindow();


protected: // メンバ関数

	void OnGui() override;


private: // メンバ関数

	void DrawGameObject(GameObject *gameObject);
	void DrawContextMenu(GameObject *gameObject);
	void DrawBackground(Scene *scene);

	void CreateEmpty(Scene *scene, GameObject *parent);
	void Delete(GameObject *gameObject);

	void AcceptDrop(GameObject *parent);
	void AcceptAssetDrop(Scene *scene, GameObject *parent);
};

} // namespace ou
