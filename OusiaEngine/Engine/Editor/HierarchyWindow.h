#pragma once

#include "Editor/EditorWindow.h"
#include "Editor/ItemClick.h"

#include <cstdint>
#include <string>
#include <unordered_set>
#include <vector>


namespace ou {

// 前方宣言
class GameObject;
class Scene;

class HierarchyWindow final: public EditorWindow {
private: // インナークラス

	// 検索欄の文字を分けたもの（"t:Light Lamp" なら types に "Light"、name に "Lamp"）
	struct SearchQuery {
		std::string name;
		std::vector<std::string> types;

		bool IsEmpty() const { return name.empty() && types.empty(); }
	};


public: // 静的メンバ関数

	// 次に Hierarchy を描くときに、gameObject の行を名前の欄にする（F2・メニューの Rename から呼ぶ）
	static void RequestRename(GameObject *gameObject);


public: // メンバ関数

	HierarchyWindow();


protected: // メンバ関数

	void OnGui() override;


private: // メンバ変数

	SelectOnRelease selectOnRelease_;

	// 名前を変えている GameObject（0 なら変えていない）
	uint32_t renamingId_ = 0;
	std::string renamingName_;
	// 次のフレームで名前の欄に入力を移す（親を開いて、行が見えるようにしてから）
	bool focusesRename_ = false;
	SlowClick<uint32_t> slowClick_;
	// 名前の欄に入力が入っていないフレームの数（入らないまま残らないように）
	int renameInactiveFrames_ = 0;

	static inline uint32_t sRenameRequestId_ = 0;

	// 検索欄（UE・Godot と同じく、ツリーの形のまま、当てはまるものとその親だけを出す）
	std::string searchText_;
	std::string previousSearchText_;
	SearchQuery searchQuery_;
	// 当てはまるもの・出すもの（当てはまるものと、その親）
	std::unordered_set<uint32_t> matchedIds_;
	std::unordered_set<uint32_t> visibleIds_;
	// 検索の文字が変わったフレームで、当てはまるものの親を開き直す
	bool opensSearchResults_ = false;
	bool focusesSearch_ = false;
	// 検索を消したあとに、見えるところまで開いてスクロールする GameObject
	uint32_t revealId_ = 0;


private: // メンバ関数

	// 上の帯の検索欄（Ctrl+F で入る。Esc か × で消す）
	void DrawSearchField();
	// 検索欄の文字から、出すものを決め直す（毎フレーム）
	void UpdateSearchResults(Scene *scene);
	bool IsSearching() const { return !searchQuery_.IsEmpty(); }
	void ClearSearch();
	// 当てはまった名前の部分に色を付ける（直前の行の上に描く）
	void DrawSearchHighlight(const GameObject *gameObject, float labelScreenX);

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
