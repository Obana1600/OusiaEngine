#pragma once

#include "Editor/EditorWindow.h"


namespace ou {

// Unity の Edit > Preferences（今は External Tools の External Script Editor だけ）
class PreferencesWindow final: public EditorWindow {
public: // メンバ関数

	PreferencesWindow();


protected: // メンバ関数

	void OnGui() override;


private: // メンバ関数

	void DrawExternalTools();
};

} // namespace ou
