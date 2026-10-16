#pragma once

#include "Graphics/GraphicsTypes.h"
#include "Scene/GameObject.h"
#include "Scene/Scene.h"

#include <memory>
#include <string_view>


namespace ou {

class GameScene {
public: // メンバ関数

	void Initialize();

	void Update();

	void Draw(Graphics::CommandList cmd) const;
	void DrawOverlay(Graphics::CommandList cmd) const;


private: // メンバ変数

	std::unique_ptr<Scene> scene_;

	GameObject *switchable_ = nullptr;
	int modelIndex_ = 0;


private: // メンバ関数

	void SwitchModel(std::string_view fileName);
};

} // namespace ou
