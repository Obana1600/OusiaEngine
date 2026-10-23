#pragma once

#include "Scene/Scene.h"
#include "Scene/SceneState.h"


namespace ou {

class TitleScene final: public SceneState {
public: // メンバ関数

	void Initialize(Scene &scene) override;
	void Update() override;
	void Finalize() override;
};

} // namespace ou
