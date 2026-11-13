#pragma once

#include "Editor/EditorWindow.h"
#include "Editor/SceneRenderTarget.h"
#include "Math/Vector2.h"


namespace ou {

class GameView final: public EditorWindow {
public: // 静的メンバ関数

	static GameView *FindVisible();


public: // メンバ関数

	GameView();

	const SceneRenderTarget &GetRenderTarget() const;
	const Vector2 &GetImageOrigin() const;
	bool HasFocus() const;
	bool IsImageHovered() const;


protected: // メンバ関数

	void OnGui() override;


private: // メンバ変数

	SceneRenderTarget renderTarget_{"GameView"};
	Vector2 imageOrigin_;
	bool hasFocus_ = false;
	bool isImageHovered_ = false;
};

} // namespace ou
