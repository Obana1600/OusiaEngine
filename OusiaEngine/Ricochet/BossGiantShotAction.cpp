#include "BossGiantShotAction.h"
#include "Boss.h"
#include "GiantBullet.h"
#include "Tuning.h"

#include <algorithm>

using namespace ou;


namespace {

constexpr float kWindup = 0.7f;
constexpr float kAfterWait = 0.6f;
constexpr float kSpeed = 9.0f;

} // namespace



bool ou::BossGiantShotAction::Update(Boss &boss, float dt) {
	timer_ += dt;
	if (!hasFired_) {
		boss.SetWarningGlow(timer_ / kWindup);
		if (timer_ < kWindup) { return false; }

		boss.SetWarningGlow(0.0f);
		hasFired_ = true;

		const auto isPhase1 = boss.GetPhase() == 1;
		const auto radius = isPhase1 ? 1.2f : 1.8f;
		// 同時に存在できる数を超えない
		const auto room = Tuning::kGiantBulletMaxCount - Projectile::CountOf(ProjectileKind::Giant);
		const auto count = std::min(isPhase1 ? 1 : 2, room);

		const auto aim = DirectionToPlayer(boss);
		for (int i = 0; i < count; i++) {
			// 2 発のときは左右に開く
			const auto angle = count == 1 ? 0.0f : (i == 0 ? -0.5f : 0.5f);
			const auto direction = RotateDirection(aim, angle);
			const auto origin = boss.GetPosition() + direction * (boss.GetRadius() + radius);
			GiantBullet::Spawn(boss.GetScene(), origin, direction * kSpeed, radius);
		}
	}
	return timer_ >= kWindup + kAfterWait;
}


void ou::BossGiantShotAction::End(Boss &boss) {
	boss.SetWarningGlow(0.0f);
}
