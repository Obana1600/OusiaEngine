#pragma once

#include "Math/Vector2.h"

#include <cstdint>


namespace ou {

// 前方宣言
class Player;

// コンボが 5 の倍数になったときの自動突進。
// 少し待ってから、ボスと反対向きに少し下がり（溜め）、引っ張りの最大速度でボスの中心へ突進する。
// 待ちから突進の終わりまでは引っ張れず、ストックも使わない。突進中は重力がかからず、ボスに当たるか中速帯に落ちたら戻る
class AutoDash {
public: // メンバ関数

	// コンボで呼ぶ。すでに動いている間は何もしない
	void Request();
	void Cancel(Player &player);

	void Update(Player &player, float dt);
	// 突進中にボスへ当たった
	void OnBossHit(Player &player);

	// 待ちから突進の終わりまで（この間は引っ張れない）
	bool IsActive() const { return state_ != State::Idle; }
	bool IsWindingUp() const { return state_ == State::WindingUp; }
	// 溜めの見た目に使う
	float GetWindUpRatio() const;
	const Vector2 &GetPullDirection() const { return pullDirection_; }


private: // インナークラス

	enum class State : uint8_t {
		Idle,
		Waiting,
		WindingUp,
		Dashing,
	};


private: // メンバ変数

	State state_ = State::Idle;
	float timer_ = 0.0f;
	Vector2 windUpFrom_;
	Vector2 windUpTo_;
	// ボスと反対向き（引っ張っている向き）
	Vector2 pullDirection_ = {-1.0f, 0.0f};


private: // メンバ関数

	void BeginWindUp(Player &player);
	void BeginDash(Player &player);
	void Finish(Player &player);
};

} // namespace ou
