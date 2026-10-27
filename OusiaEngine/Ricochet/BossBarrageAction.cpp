#include "BossBarrageAction.h"
#include "Boss.h"
#include "Math/Random.h"
#include "SmallBullet.h"

using namespace ou;


namespace {

// 撃つ前の溜め
constexpr float kWindup = 0.4f;
constexpr float kDuration = 1.4f;
constexpr float kSpread = 0.6f;

} // namespace



void ou::BossBarrageAction::Begin(Boss &) {
	timer_ = 0.0f;
	fireTimer_ = 0.0f;
}


bool ou::BossBarrageAction::Update(Boss &boss, float dt) {
	timer_ += dt;
	if (timer_ < kWindup) {
		boss.SetWarningGlow(timer_ / kWindup);
		return false;
	}
	boss.SetWarningGlow(0.0f);

	// 第二形態は間隔が短く、弾が速い
	const auto interval = boss.GetPhase() == 1 ? 0.07f : 0.05f;
	const auto speed = boss.GetPhase() == 1 ? 11.0f : 13.0f;

	fireTimer_ -= dt;
	while (fireTimer_ <= 0.0f) {
		fireTimer_ += interval;
		const auto direction = RotateDirection(DirectionToPlayer(boss), Random::Range(-kSpread, kSpread));
		const auto origin = boss.GetPosition() + direction * boss.GetRadius();
		SmallBullet::Spawn(boss.GetScene(), origin, direction * speed);
	}
	return timer_ >= kWindup + kDuration;
}


void ou::BossBarrageAction::End(Boss &boss) {
	boss.SetWarningGlow(0.0f);
}
