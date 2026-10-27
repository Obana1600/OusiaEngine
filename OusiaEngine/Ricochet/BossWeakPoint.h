#pragma once

#include "Math/Vector2.h"
#include "Scene/Component.h"

#include <memory>


namespace ou {

// 前方宣言
class Material;
class Transform;

// 第二形態の弱点。常に露出していて、一定時間で位置（ボスの周りの角度）が切り替わる。切り替わる前は点滅して予告する
class BossWeakPoint final: public Component {
public: // メンバ関数

	void Start() override;
	void Update() override;

	void Activate();
	bool IsActive() const { return isActive_; }

	// direction はボスの中心から当たった位置への向き
	bool IsHit(const Vector2 &direction) const;


private: // メンバ変数

	bool isActive_ = false;
	float angle_ = 0.0f;
	float switchTimer_ = 0.0f;

	Transform *marker_ = nullptr;
	std::shared_ptr<Material> markerMaterial_;


private: // メンバ関数

	void MoveToNewAngle();
};

} // namespace ou
