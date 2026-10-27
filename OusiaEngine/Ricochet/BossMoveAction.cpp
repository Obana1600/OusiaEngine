#include "BossMoveAction.h"
#include "Boss.h"
#include "Math/Mathf.h"
#include "Math/Random.h"
#include "Player.h"
#include "Tuning.h"

using namespace ou;


namespace {

constexpr float kWarnTime = 0.6f;
constexpr float kMoveTime = 0.5f;
// 今の位置から最低これだけ離れた場所へ動く
constexpr float kMinDistance = 6.0f;
// プレイヤーの真上には移動しない
constexpr float kPlayerClearance = 4.0f;

} // namespace



void ou::BossMoveAction::Begin(Boss &boss) {
	from_ = boss.GetPosition();
	to_ = ChooseDestination(boss);
	line_.Create(boss.GetScene(), 0.3f, {1.0f, 0.85f, 0.3f, 0.8f});
	const Vector2 points[] = {from_, to_};
	line_.SetPoints(points);
}


bool ou::BossMoveAction::Update(Boss &boss, float dt) {
	timer_ += dt;
	if (timer_ < kWarnTime) {
		line_.SetAlpha(0.5f + 0.5f * Mathf::Sin(timer_ * 30.0f));
		return false;
	}
	line_.Destroy();

	// smoothstep で動き出しと止まりをなめらかにする
	const auto t = Mathf::Clamp01((timer_ - kWarnTime) / kMoveTime);
	const auto eased = t * t * (3.0f - 2.0f * t);
	boss.SetPosition(Vector2::Lerp(from_, to_, eased));
	return t >= 1.0f;
}


void ou::BossMoveAction::End(Boss &) {
	line_.Destroy();
}


Vector2 ou::BossMoveAction::ChooseDestination(const Boss &boss) {
	const auto marginX = Tuning::kArenaHalfWidth - boss.GetRadius() - 2.0f;
	const auto marginY = Tuning::kArenaHalfHeight - boss.GetRadius() - 1.5f;
	const auto *player = Player::GetInstance();

	Vector2 best = boss.GetPosition();
	for (int i = 0; i < 16; i++) {
		const Vector2 candidate = {Random::Range(-marginX, marginX), Random::Range(-marginY, marginY)};
		if (Vector2::Distance(candidate, boss.GetPosition()) < kMinDistance) { continue; }
		if (player && Vector2::Distance(candidate, player->GetPosition()) < kPlayerClearance + boss.GetRadius()) { continue; }
		return candidate;
	}
	return best;
}
