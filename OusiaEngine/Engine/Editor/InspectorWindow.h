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
class Object;

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

	// アセットを選んでいるとき
	std::string inspectedAssetPath_;
	std::string inspectedSubAsset_;
	uint32_t inspectedAssetId_ = 0;
	bool isFolder_ = false;
	std::unique_ptr<Editor> assetEditor_;

	// Preview の欄（Unity と同じく Inspector の下。境目をドラッグして高さを変えられる）
	float previewHeight_ = 240.0f;
	bool isPreviewOpen_ = true;


private: // メンバ関数

	void DrawGameObjectInspector(GameObject *gameObject);
	void RebuildEditorsIfNeeded(GameObject *gameObject);
	void ClearGameObjectEditors();

	void DrawAssetInspector();
	void RebuildAssetEditorIfNeeded(Object *asset);
	void ClearAssetEditor();
	void DrawAssetHeader(Object *asset);
	void DrawPreview(Editor *editor);

	void DrawGameObjectHeader(GameObject *gameObject);
	void DrawComponent(Editor *editor);
	void DrawComponentMenu(Component *component);
	void DrawAddComponentButton(GameObject *gameObject);
	void DrawAddScriptItems(GameObject *gameObject);
};

} // namespace ou
