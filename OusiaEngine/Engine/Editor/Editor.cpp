#include "Editor.h"
#include "Editor/CustomEditor.h"
#include "Core/Object.h"
#include "Editor/EditorGuiLayout.h"

using namespace ou;


std::unique_ptr<Editor> ou::Editor::Create(Object *target) {
	if (!target) { return nullptr; }

	auto editor = CustomEditor::Create(*target);
	if (!editor) { editor.reset(new Editor()); }

	editor->target_ = target;
	editor->serializedObject_ = std::make_unique<SerializedObject>(target);
	editor->OnEnable();
	return editor;
}


void ou::Editor::OnInspectorGui() {
	DrawDefaultInspector();
}


bool ou::Editor::DrawDefaultInspector() {
	serializedObject_->Update();

	auto property = serializedObject_->GetIterator();
	for (auto enterChildren = true; property.NextVisible(enterChildren); enterChildren = false) {
		EditorGuiLayout::PropertyField(property, true);
	}

	return serializedObject_->ApplyModifiedProperties();
}


std::string ou::Editor::GetPreviewTitle() const {
	return target_ ? target_->GetName() : std::string();
}


Object *ou::Editor::GetTarget() const {
	return target_;
}


SerializedObject *ou::Editor::GetSerializedObject() {
	return serializedObject_.get();
}
