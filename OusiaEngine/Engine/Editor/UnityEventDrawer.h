#pragma once

#include <string>
#include <string_view>


namespace ou {

// 前方宣言
class MonoBehavior;
class SerializedObject;

// Unity の UnityEventDrawer（Inspector の UnityEvent の一覧。対象のスクリプトと、呼ぶメソッド・引数を選ぶ）
class UnityEventDrawer final {
public: // 静的メンバ関数

	UnityEventDrawer() = delete;

	// path は UnityEvent を保存している配列（Button なら "onClick"）
	static void Draw(SerializedObject &so, std::string_view path, std::string_view label);


private: // 静的メンバ関数

	static void DrawCall(SerializedObject &so, const std::string &elementPath, bool &isRemoved);
	static void DrawMethodPopup(SerializedObject &so, const std::string &elementPath);
	static void RemoveCall(SerializedObject &so, const std::string &path, int index);
	static std::string FindScriptClassName(const MonoBehavior *behavior);
};

} // namespace ou
