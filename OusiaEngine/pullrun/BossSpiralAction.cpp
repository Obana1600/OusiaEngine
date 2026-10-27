#include "BossSpiralAction.h"
#include "Boss.h"
#include "Math/Mathf.h"
#include "Math/Random.h"
#include "SmallBullet.h"

using namespace ou;


namespace {

constexpr float kWindup = 0.5f;
constexpr float kDuration = 2.6f;
constexpr float kInterval = 0.06f;
constexpr int kArms = 3;
// 1 秒に回る角度（ラジアン）
constexpr float kSpinSpeed = 2.4f;
constexpr float kBulletSpeed = 9.0f;

} // namespace



void ou::BossSpiralAction::Begin(Boss &) {
	angle_ = Random::Range(0.0f, 2.0f * Mathf::PI);
	spinSign_ = Random::Value() < 0.5f ? -1.0f : 1.0f;
}


bool ou::BossSpiralAction::Update(Boss &boss, float dt) {
	timer_ += dt;
	if (timer_ < kWindup) {
		boss.SetWarningGlow(timer_ / kWindup);
		return false;
	}
	boss.SetWarningGlow(0.0f);

	angle_ += kSpinSpeed * spinSign_ * dt;
	fireTimer_ -= dt;
	while (fireTimer_ <= 0.0f) {
		fireTimer_ += kInterval;
		for (int arm = 0; arm < kArms; arm++) {
			const auto angle = angle_ + 2.0f * Mathf::PI * static_cast<float>(arm) / static_cast<float>(kArms);
			const Vector2 direction = {Mathf::Cos(angle), Mathf::Sin(angle)};
			const auto origin = boss.GetPosition() + direction * boss.GetRadius();
			SmallBullet::Spawn(boss.GetScene(), origin, direction * kBulletSpeed);
		}
	}
	return timer_ >= kWindup + kDuration;
}


void ou::BossSpiralAction::End(Boss &boss) {
	boss.SetWarningGlow(0.0f);
}
