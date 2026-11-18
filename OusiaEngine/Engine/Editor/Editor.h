#pragma once

#include "Editor/SerializedObject.h"
#include "Math/Rect.h"

#include <memory>
#include <string>


namespace ou {

// 前方宣言
class Object;


class Editor {
public: // 静的メンバ関数

	static std::unique_ptr<Editor> Create(Object *target);


public: // メンバ関数

	Editor(const Editor &) = delete;
	Editor &operator=(const Editor &) = delete;
	virtual ~Editor() = default;

	virtual void OnInspectorGui();

	bool DrawDefaultInspector();

	// ---- Preview（Inspector の下の欄。Unity の Editor.HasPreviewGUI / OnPreviewGUI） ----

	// Preview の欄を出すか
	virtual bool HasPreviewGui() const { return false; }

	// Preview の欄の見出し（既定は対象の名前）
	virtual std::string GetPreviewTitle() const;

	// Preview の見出しの右に並べるボタンなど（再生ボタンなど）。ImGui の SameLine で横に並ぶ
	virtual void OnPreviewSettings() {}

	// Preview の中身を描く。rect はスクリーン座標（ImGui::GetWindowDrawList に描くか、カーソルを置いて部品を置く）
	virtual void OnPreviewGui(const Rect &rect) { (void)rect; }

	// ---- Scene ビュー（Unity の Editor.OnSceneGUI） ----

	// 選んでいる GameObject のコンポーネントの Editor が、Scene ビューに描く。sceneRect は Scene ビューの絵の範囲（スクリーン座標）
	virtual void OnSceneGui(const Rect &sceneRect) { (void)sceneRect; }

	Object *GetTarget() const;
	SerializedObject *GetSerializedObject();


protected: // メンバ関数

	Editor() = default;

	virtual void OnEnable() {};


private: // メンバ変数

	Object *target_ = nullptr;
	std::unique_ptr<SerializedObject> serializedObject_;
};

} // namespace ou
