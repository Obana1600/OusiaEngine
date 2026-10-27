#include "BossDashAction.h"
#include "Boss.h"
#include "CameraRig.h"
#include "GameColors.h"
#include "GameAudio.h"
#include "ParticleEffects.h"
#include "Math/Mathf.h"
#include "Motion2D.h"

using namespace ou;


namespace {

constexpr float kWarnTime = 0.9f;
constexpr float kDashSpeed = 30.0f;
constexpr float kMaxDashTime = 1.6f;
constexpr int kMaxBounces = 3;
// 突進終わりは長時間止まる（反撃の時間）
constexpr float kRecoverTime = 2.2f;

MotionParams MakeDashParams(const Boss &boss) {
	MotionParams params;
	params.radius = boss.GetRadius();
	return params;
}

} // namespace



void ou::BossDashAction::Begin(Boss &boss) {
	GameAudio::PlaySe(Se::BossCharge);
	direction_ = DirectionToPlayer(boss);

	// 本番の突進と同じ移動処理で軌道を出す
	const MotionState state = {boss.GetPosition(), direction_ * kDashSpeed};
	const auto points = Motion2D::Trace(state, MakeDashParams(boss), kMaxDashTime, 1.0f / 60.0f, kMaxBounces);
	line_.Create(boss.GetScene(), boss.GetRadius() * 1.2f, {1.0f, 0.2f, 0.2f, 0.45f});
	line_.SetPoints(points);
}


bool ou::BossDashAction::Update(Boss &boss, float dt) {
	timer_ += dt;

	switch (step_) {
		case Step::Warning:
			boss.SetWarningGlow(timer_ / kWarnTime);
			line_.SetAlpha(0.6f + 0.4f * Mathf::Sin(timer_ * 25.0f));
			if (timer_ >= kWarnTime) {
				line_.Destroy();
				boss.SetWarningGlow(0.0f);
				boss.SetDashing(true);
				boss.SetDashVelocity(direction_ * kDashSpeed);
				step_ = Step::Dashing;
				timer_ = 0.0f;
			}
			return false;

		case Step::Dashing: {
			// 高速のプレイヤーとぶつかると CombatSystem が向きを変える
			MotionState state = {boss.GetPosition(), boss.GetDashVelocity()};
			const auto result = Motion2D::Step(state, MakeDashParams(boss), dt);
			bounceCount_ += result.bounceCount;
			if (result.bounceCount > 0) {
				// 壁にぶつかるたびに大きくはじけて揺れる
				const auto contact = state.position - result.lastWallNormal * boss.GetRadius();
				ParticleEffects::PlayHit(contact, result.lastWallNormal, GameColors::kBossDanger, 1.0f);
				if (auto *camera = CameraRig::GetInstance()) { camera->Shake(0.4f); }
			}
			boss.SetPosition(state.position);
			boss.SetDashVelocity(state.velocity);
			if (bounceCount_ >= kMaxBounces || timer_ >= kMaxDashTime) {
				boss.SetDashing(false);
				boss.SetDashVelocity(Vector2::Zero);
				step_ = Step::Recovering;
				timer_ = 0.0f;
			}
			return false;
		}

		case Step::Recovering:
			return timer_ >= kRecoverTime;
	}
	return true;
}


void ou::BossDashAction::End(Boss &boss) {
	line_.Destroy();
	boss.SetDashing(false);
	boss.SetDashVelocity(Vector2::Zero);
	boss.SetWarningGlow(0.0f);
}
