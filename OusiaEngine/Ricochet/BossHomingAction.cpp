#include "BossHomingAction.h"
#include "Boss.h"
#include "HomingBullet.h"
#include "Math/Mathf.h"

using namespace ou;


namespace {

constexpr float kWindup = 0.5f;
constexpr float kInterval = 0.2f;
constexpr float kAfterWait = 0.5f;

} // namespace



bool ou::BossHomingAction::Update(Boss &boss, float dt) {
	timer_ += dt;
	if (timer_ < kWindup) {
		boss.SetWarningGlow(timer_ / kWindup);
		return false;
	}
	boss.SetWarningGlow(0.0f);

	const auto isPhase1 = boss.GetPhase() == 1;
	const auto count = isPhase1 ? 3 : 5;
	const auto speed = isPhase1 ? 8.0f : 10.0f;
	const auto turnRate = isPhase1 ? 1.8f : 3.0f;
	const auto homingTime = isPhase1 ? 2.5f : 3.0f;

	while (firedCount_ < count && timer_ >= kWindup + kInterval * static_cast<float>(firedCount_)) {
		// プレイヤーと反対寄りの扇に撃って、回り込んでくるようにする
		const auto t = count == 1 ? 0.5f : static_cast<float>(firedCount_) / static_cast<float>(count - 1);
		const auto angle = (t - 0.5f) * 2.4f + Mathf::PI;
		const auto direction = RotateDirection(DirectionToPlayer(boss), angle);
		const auto origin = boss.GetPosition() + direction * boss.GetRadius();
		HomingBullet::Spawn(boss.GetScene(), origin, direction * speed, turnRate, homingTime);
		firedCount_++;
	}
	return firedCount_ >= count && timer_ >= kWindup + kInterval * static_cast<float>(count) + kAfterWait;
}


void ou::BossHomingAction::End(Boss &boss) {
	boss.SetWarningGlow(0.0f);
}
