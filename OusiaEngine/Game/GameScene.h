#pragma once

#include "Graphics/GraphicsTypes.h"
#include "Scene/Scene.h"

#include <memory>


namespace ou {

class GameScene {
public: // メンバ関数

	void Initialize();

	void Update();

	void Draw(CommandList cmd) const;


private: // メンバ変数

	std::unique_ptr<Scene> scene_;


private: // メンバ関数

};

} // namespace ou