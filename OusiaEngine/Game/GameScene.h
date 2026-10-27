#pragma once

#include "Scene/Component.h"
#include "Scene/Scene.h"

#include <vector>


namespace ou {

class Graphic;

void BuildGameScene(Scene &scene);

// 単位 4: EventSystem と Button を試す
class UiInputTest final: public Component {
public: // メンバ関数

	void Start() override;
	void Update() override;


private: // メンバ変数

	struct Watched {
		Graphic *graphic = nullptr;
		const void *vertices = nullptr;
	};

	std::vector<Watched> watched_;
	GameObject *lastSelected_ = nullptr;
	int frame_ = 0;
};

} // namespace ou
