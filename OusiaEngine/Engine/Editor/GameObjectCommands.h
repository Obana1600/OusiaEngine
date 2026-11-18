#pragma once

#include "Core/JsonFile.h"

#include <cstdint>
#include <string>


namespace ou {

// 前方宣言
class GameObject;

// Unity の Edit メニューの Copy / Paste / Paste As Child / Duplicate / Rename / Delete（シーンの GameObject）
// コピーはエディタの中だけに取っておく（OS のクリップボードは使わない。Unity と同じ）
// Play 中は Delete のほかは使えない（Stop すると消えるため。GameObject メニューにそろえる）
class GameObjectCommands final {
public: // 静的メンバ関数

	GameObjectCommands() = delete;

	static bool CanCopy(const GameObject *gameObject);
	static bool CanPaste();
	static bool CanPasteAsChild(const GameObject *parent);
	static bool CanDuplicate(const GameObject *gameObject);
	static bool CanDelete(const GameObject *gameObject);
	static bool CanRename(const GameObject *gameObject);

	// gameObject と子を、今の形で取っておく
	static void Copy(GameObject *gameObject);
	// 取っておいたものを、元と同じ親の下に置く（元の親がなければシーンの一番上）
	static GameObject *Paste();
	// 取っておいたものを、parent の子の最後に置く
	static GameObject *PasteAsChild(GameObject *parent);
	// 同じ親の、すぐ下に置く
	static GameObject *Duplicate(GameObject *gameObject);
	static void Delete(GameObject *gameObject);

	// Edit メニューと、Hierarchy の右クリックのメニューの項目（target は選んでいるもの。nullptr なら Paste だけ使える）
	static void DrawMenuItems(GameObject *target);

	// Ctrl+C / Ctrl+V / Ctrl+Shift+V / Ctrl+D / Delete / F2。ウィンドウの OnGui の中で呼ぶ（そのウィンドウを選んでいるときだけ効く）
	static void HandleShortcuts();

	// 兄弟（self は除く）に同じ名前があるときだけ、重ならないいちばん小さい番号を付ける（"Cube" → "Cube (1)"）
	static std::string MakeUniqueName(const std::string &name, const GameObject *self);


private: // 静的メンバ変数

	static inline Json sClipboard_;
	static inline bool sHasClipboard_ = false;
	// コピーした元と、その親（ペーストする場所を決める。消えていれば使わない）
	static inline uint32_t sSourceId_ = 0;
	static inline uint32_t sSourceParentId_ = 0;
};

} // namespace ou
