#pragma once

#include "Graphics/GraphicsTypes.h"
#include "2D/Sprite.h"

#include <memory>


namespace ou {

class GameScene {
public: // メンバ関数

	void Initialize();

	void Update();

	void Draw(CommandList cmd) const;


private: // メンバ変数

	std::unique_ptr<Sprite> sprite_;


private: // メンバ関数

};

} // namespace ou