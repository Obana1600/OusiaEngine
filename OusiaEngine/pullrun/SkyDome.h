#pragma once

#include "Math/Color.h"
#include "Scene/Component.h"

#include <memory>


namespace ou {

// 前方宣言
class Material;
class Transform;

// ステージの奥に見える天球（星雲と星）。ゆっくり回り、第二形態では赤みを帯びる
class SkyDome final: public Component {
public: // メンバ関数

	void Start() override;
	void Update() override;


private: // メンバ変数

	Transform *sphere_ = nullptr;
	std::shared_ptr<Material> material_;
	Color color_ = Color::White;
	float angle_ = 0.0f;
};

} // namespace ou
