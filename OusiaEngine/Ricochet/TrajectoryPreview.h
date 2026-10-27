#pragma once

#include "Scene/Component.h"


namespace ou {

// 前方宣言
class LineRenderer;

// 溜め中に、今発射したときの軌道を表示する。本番と同じ移動処理（Motion2D）で計算し、反射は壁だけ考える
class TrajectoryPreview final: public Component {
public: // メンバ関数

	void Start() override;
	void LateUpdate() override;


private: // メンバ変数

	LineRenderer *line_ = nullptr;
};

} // namespace ou
