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


private: // メンバ変数

	// 行の矢印を押したか（矢印で開いたときは選ばない）
	bool isPressedOnArrow_ = false;


private: // メンバ関数

	void DrawGameObject(GameObject *gameObject);
	void DrawContextMenu(GameObject *gameObject);
	void DrawBackground(Scene *scene);

	void Delete(GameObject *gameObject);

	void AcceptDropOnRow(GameObject *target);
	void AcceptDropOnBackground();
	void AcceptAssetDrop(Scene *scene, GameObject *parent);
};

} // namespace ou
