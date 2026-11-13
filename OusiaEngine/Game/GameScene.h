#pragma once

#include "Scene/Component.h"
#include "Scene/Scene.h"


namespace ou {

void BuildGameScene(Scene &scene);

// 確認用。数フレーム後に終了する
class QuitAfterFrames final: public Component {
public:

	void Update() override;
};

// 確認用（エディタの単位 6-3）。ゲームに届いたキーとクリックをログに出す
class InputProbe final: public Component {
public:

	void Update() override;
};

} // namespace ou
