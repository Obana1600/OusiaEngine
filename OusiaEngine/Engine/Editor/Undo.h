#pragma once

#include "Core/JsonFile.h"

#include <string>
#include <string_view>


namespace ou {

// 前方宣言
class Scene;

// Unity の Undo。操作のたびにシーンを json に書いて GameObject ごとに比べ、変わった GameObject の前と後だけを残す
// 戻すときはシーンを作り直さず、今ある GameObject・Component に読み直させる（ほかからの参照や選択が残るように）
// シーンを変えたところは EditorSceneManager::MarkSceneDirty を呼ぶので、そこで変わったことを知る
// Unity のように操作の前に Undo.RecordObject を呼んでもらう形にしないのは、呼び忘れたところで戻せなくなるため
// ドラッグのあいだは毎フレーム変わるので、マウスを離して入力が終わったときに 1 つの操作としてまとめる
class Undo final {
	friend class Engine;
	friend class EditorApplication;
	friend class EditorMainWindow;
	friend class EditorSceneManager;

public: // 静的メンバ関数

	Undo() = delete;

	// これから記録する操作の名前（Edit メニューの "Undo ○○" に出る）。付けなければ "Change"
	static void SetCurrentGroupName(std::string_view name);

	// 次のフレームの頭で戻す・やり直す（ウィンドウを描いている途中でシーンを入れ替えないため）
	static void PerformUndo();
	static void PerformRedo();

	static bool CanUndo();
	static bool CanRedo();

	// 次に戻す・やり直す操作の名前（なければ空）
	static std::string GetUndoName();
	static std::string GetRedoName();

	// 履歴を全部消す
	static void ClearAll();


private: // 静的メンバ関数

	// シーンを開いた・作ったときに、履歴を消して今のシーンを始まりにする
	static void ResetHistory(Scene *scene, bool isSaved);
	// 履歴は残したまま、今のシーンを記録し直す（スクリプトを読み直して、書き出す中身が変わったときなど）
	static void RefreshCurrent(Scene *scene);

	// EditorSceneManager::MarkSceneDirty から呼ぶ
	static void NotifySceneChanged(Scene *scene);
	// EditorSceneManager::SaveScene から呼ぶ
	static void NotifySceneSaved(Scene *scene);

	// 操作が終わっていれば記録する（force なら、ドラッグの途中でも記録する）
	static void FlushPendingChange(bool force);

	// Engine::Update から、ImGui のフレームの外で呼ぶ
	static void ApplyPendingRequest();
};

} // namespace ou
