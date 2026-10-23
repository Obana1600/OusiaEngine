#pragma once

#include "Framework.h"


namespace ou {

class MyGame final: public Framework {
public: // メンバ関数

	MyGame();


protected: // メンバ関数

	void Initialize() override;
};

} // namespace ou
