#pragma once

#include "Editor/GuiContent.h"
#include "Editor/SerializedProperty.h"

#include <span>
#include <string_view>


namespace ou {

class EditorGuiLayout final {
public: // 静的メンバ関数

	EditorGuiLayout() = delete;

	static void PrefixLabel(std::string_view label);
	static void PrefixLabel(const GuiContent &label); // ツールチップがあれば、マウスを乗せたときに出す

	// Unity の EditorGuiLayout.PropertyField。値の型に合った欄を 1 つ描き、変えたら SerializedObject に書く
	// includeChildren が true なら、入れ子・配列の中身も描く
	// 戻り値は Unity と同じく「子があり、開いていて、includeChildren が false」なら true（呼ぶ側が子を描く）
	static bool PropertyField(SerializedProperty property, bool includeChildren = false);
	static bool PropertyField(SerializedProperty property, std::string_view label, bool includeChildren = false);
	static bool PropertyField(SerializedProperty property, const GuiContent &label, bool includeChildren = false);

	// Unity の EditorGuiLayout.Popup。選択肢から 1 つ選ぶ欄。選んだ番号を返す（変えなければ selectedIndex のまま）
	// 選択肢のツールチップは、開いた一覧で選択肢にマウスを乗せたときと、閉じた欄に乗せたとき（今の値）に出す
	static int Popup(const GuiContent &label, int selectedIndex, std::span<const GuiContent> displayedOptions);
};

} // namespace ou
