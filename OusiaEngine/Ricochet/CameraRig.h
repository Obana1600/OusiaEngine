#pragma once

#include "Math/Vector2.h"
#include "Scene/Component.h"


namespace ou {

// 前方宣言
class Camera;

// 通常はプレイヤーに寄り、溜めるほど引いて、最大溜めでステージ全体を映す。切り替えはすべて補間する。
// 発射後は速さが再発射閾値以下になるまで引いたままにする
class CameraRig final: public Component {
public: // 静的メンバ関数

	static CameraRig *GetInstance() { return sInstance_; }


public: // メンバ関数

	void Awake() override;
	void LateUpdate() override;
	void OnDestroy() override;

	// 画面の揺れとパッドの振動
	void Shake(float strength);


private: // 静的メンバ変数

	static inline CameraRig *sInstance_ = nullptr;


private: // メンバ変数

	Camera *camera_ = nullptr;
	// 0 で寄り、1 で全体
	float zoom_ = 0.0f;
	// 発射したときの引き具合。速さが落ちるまで保つ
	float heldZoom_ = 0.0f;
	Vector2 center_;
	float viewHeight_ = 0.0f;
	float shake_ = 0.0f;


private: // メンバ関数

	float ComputeFullViewHeight() const;
};

} // namespace ou
