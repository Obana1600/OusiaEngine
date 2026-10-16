#pragma once

#include "Graphics/GraphicsTypes.h"
#include "Scene/GameObject.h"
#include "Scene/Scene.h"

#include <memory>


namespace ou {

// 前方宣言
class Image;

class GameScene {
public: // メンバ関数

	void Initialize();

	void Update();

	void Draw(Graphics::CommandList cmd) const;
	void DrawOverlay(Graphics::CommandList cmd) const;


private: // メンバ変数

	std::unique_ptr<Scene> scene_;

	Image *uvChecker_ = nullptr;


private: // メンバ関数

	// uvChecker のプロパティを ImGui で操作する
	void ShowUvCheckerWindow();
};

} // namespace ou
