#pragma once

#include "Editor/EditorWindow.h"
#include "Editor/SlowClick.h"

#include <cstdint>
#include <string>


namespace ou {

// 前方宣言
class GameObject;
class Scene;

class HierarchyWindow final: public EditorWindow {
public: // 静的メンバ関数

	// 次に Hierarchy を描くときに、gameObject の行を名前の欄にする（F2・メニューの Rename から呼ぶ）
	static void RequestRename(GameObject *gameObject);


public: // メンバ関数

	HierarchyWindow();


protected: // メンバ関数

	void OnGui() override;


private: // メンバ変数

	// 行の矢印を押したか（矢印で開いたときは選ばない）
	bool isPressedOnArrow_ = false;

	// 名前を変えている GameObject（0 なら変えていない）
	uint32_t renamingId_ = 0;
	std::string renamingName_;
	// 次のフレームで名前の欄に入力を移す（親を開いて、行が見えるようにしてから）
	bool focusesRename_ = false;
	SlowClick<uint32_t> slowClick_;
	// 名前の欄に入力が入っていないフレームの数（入らないまま残らないように）
	int renameInactiveFrames_ = 0;

	static inline uint32_t sRenameRequestId_ = 0;


private: // メンバ関数

	void DrawGameObject(GameObject *gameObject);
	void DrawContextMenu(GameObject *gameObject);

	void BeginRename(GameObject *gameObject);
	// 行の名前の欄。labelX は名前を出す位置
	void DrawRenameField(GameObject *gameObject, float labelX);
	void EndRename(bool commits);
	void DrawBackground(Scene *scene);


	void AcceptDropOnRow(GameObject *target);
	void AcceptDropOnBackground();
	void AcceptAssetDrop(Scene *scene, GameObject *parent);
};

} // namespace ou
