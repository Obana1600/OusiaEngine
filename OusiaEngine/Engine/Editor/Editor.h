#pragma once

#include "Editor/SerializedObject.h"

#include <memory>


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
