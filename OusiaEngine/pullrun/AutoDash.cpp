#include "AutoDash.h"
#include "Boss.h"
#include "GameAudio.h"
#include "GameColors.h"
#include "ParticleEffects.h"
#include "Math/Mathf.h"
#include "Player.h"
#include "Tuning.h"

#include <algorithm>

using namespace ou;



void ou::AutoDash::Request() {
	if (IsActive()) { return; }
	state_ = State::Waiting;
	timer_ = Tuning::kAutoDashDelay;
}


void ou::AutoDash::Cancel(Player &player) {
	if (!IsActive()) { return; }
	Finish(player);
}


void ou::AutoDash::Update(Player &player, float dt) {
	switch (state_) {
		case State::Idle:
			break;

		case State::Waiting:
			timer_ -= dt;
			if (timer_ <= 0.0f) { BeginWindUp(player); }
			break;

		case State::WindingUp: {
			// 止まって、ボスと反対向きにゆっくり下がる
			timer_ += dt;
			const auto t = Mathf::Clamp01(timer_ / Tuning::kAutoDashWindUpTime);
			const auto eased = 1.0f - (1.0f - t) * (1.0f - t);
			player.SetPosition(Vector2::Lerp(windUpFrom_, windUpTo_, eased));
			player.SetVelocity(Vector2::Zero);
			if (t >= 1.0f) { BeginDash(player); }
			break;
		}

		case State::Dashing:
			// 中速帯に落ちたら終わり
			if (player.GetBand() != SpeedBand::High) { Finish(player); }
			break;
	}
}


void ou::AutoDash::OnBossHit(Player &player) {
	if (state_ == State::Dashing) { Finish(player); }
}


float ou::AutoDash::GetWindUpRatio() const {
	if (state_ != State::WindingUp) { return 0.0f; }
	return Mathf::Clamp01(timer_ / Tuning::kAutoDashWindUpTime);
}


void ou::AutoDash::BeginWindUp(Player &player) {
	const auto *boss = Boss::GetInstance();
	if (!boss || boss->GetState() != BossState::Fighting) {
		Finish(player);
		return;
	}

	const auto toBoss = boss->GetPosition() - player.GetPosition();
	pullDirection_ = toBoss.LengthSquared() > 0.0001f ? -toBoss.Normalized() : Vector2{-1.0f, 0.0f};

	// アリーナの外へは下がらない
	const auto limitX = Tuning::kArenaHalfWidth - Tuning::kPlayerRadius;
	const auto limitY = Tuning::kArenaHalfHeight - Tuning::kPlayerRadius;
	windUpFrom_ = player.GetPosition();
	windUpTo_ = windUpFrom_ + pullDirection_ * Tuning::kAutoDashBackOff;
	windUpTo_.x = std::clamp(windUpTo_.x, -limitX, limitX);
	windUpTo_.y = std::clamp(windUpTo_.y, -limitY, limitY);

	state_ = State::WindingUp;
	timer_ = 0.0f;
	player.SetGravityEnabled(false);
	// 自動突進の合図
	ParticleEffects::PlayRing(player.GetPosition(), GameColors::kGold, 2.5f);
}


void ou::AutoDash::BeginDash(Player &player) {
	const auto *boss = Boss::GetInstance();
	if (!boss || boss->GetState() != BossState::Fighting) {
		Finish(player);
		return;
	}

	// 溜めている間にボスが動いても、今のボスの中心を狙う
	const auto toBoss = boss->GetPosition() - player.GetPosition();
	const auto direction = toBoss.LengthSquared() > 0.0001f ? toBoss.Normalized() : -pullDirection_;
	player.SetVelocity(direction * Tuning::kPlayerMaxSpeed);
	ParticleEffects::PlayHit(player.GetPosition(), -direction, GameColors::kGold, 1.0f);
	GameAudio::PlaySe(Se::PlayerDash);
	state_ = State::Dashing;
}


void ou::AutoDash::Finish(Player &player) {
	state_ = State::Idle;
	timer_ = 0.0f;
	player.SetGravityEnabled(true);
}
