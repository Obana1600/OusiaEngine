#pragma once

#include "Scene/Component.h"


namespace ou {

// ゲームの Component を TypeRegistry に登録する（.scene を読む前に呼ぶ）
void RegisterGameTypes();

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
