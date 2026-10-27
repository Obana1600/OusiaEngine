#include "Player.h"
#include "Core/Time.h"
#include "Math/Mathf.h"
#include "PlayerVisual.h"
#include "Scene/GameObject.h"
#include "TimeControl.h"
#include "Tuning.h"

#include <algorithm>

using namespace ou;



MotionParams ou::Player::MakeMotionParams() {
	MotionParams params;
	params.radius = Tuning::kPlayerRadius;
	params.drag = Tuning::kPlayerDrag;
	params.restitution = Tuning::kWallRestitution;
	params.gravityBelowSpeed = Tuning::kDamageSpeed;
	params.gravity = Tuning::kGravity;
	return params;
}


void ou::Player::Awake() {
	sInstance_ = this;
	hp_ = Tuning::kPlayerMaxHp;
	stock_ = Tuning::kMaxStock;
	motion_.position = {-10.0f, -6.0f};
}


void ou::Player::Start() {
	visual_ = GetGameObject()->GetComponent<PlayerVisual>();
}


void ou::Player::Update() {
	const auto dt = Time::GetDeltaTime();

	if (isFrozen_) {
		TimeControl::SetSlowMotion(false);
		return;
	}

	// 再発射は速さが再発射閾値以下で、ストックが残っているときだけ
	const auto canStart = !IsDead() && CanRelaunch(GetBand()) && stock_ > 0;
	pull_.Update(Time::GetUnscaledDeltaTime(), canStart);
	if (pull_.IsFired()) { Launch(); }
	TimeControl::SetSlowMotion(pull_.IsCharging());

	// ストックは 1 つずつ自動回復する
	if (stock_ < Tuning::kMaxStock) {
		stockTimer_ += dt;
		if (stockTimer_ >= Tuning::kStockRecoverTime) {
			stockTimer_ = 0.0f;
			stock_++;
		}
	}

	const auto result = Motion2D::Step(motion_, MakeMotionParams(), dt);
	if (result.bounceCount > 0 && visual_) { visual_->OnImpact(result.lastWallNormal, result.wallImpulse); }

	invincibleTimer_ = std::max(invincibleTimer_ - dt, 0.0f);

	const auto band = GetBand();
	if (band != lastBand_ && visual_) { visual_->OnBandChanged(band); }
	lastBand_ = band;
}


void ou::Player::OnDestroy() {
	if (sInstance_ == this) { sInstance_ = nullptr; }
}


float ou::Player::GetRadius() const {
	return Tuning::kPlayerRadius;
}


Vector2 ou::Player::ComputeLaunchVelocity() const {
	const auto ratio = Mathf::Lerp(Tuning::kMinLaunchRatio, 1.0f, pull_.GetChargeRatio());
	return -pull_.GetPullDirection() * (Tuning::kPlayerMaxSpeed * ratio);
}


bool ou::Player::TakeHit(float damage, const Vector2 &knockbackDirection) {
	if (IsInvincible() || IsDead() || isFrozen_) { return false; }

	if (HasSuperArmor(GetBand())) {
		// スーパーアーマー：ダメージ軽減＋吹っ飛び無効
		hp_ -= damage * Tuning::kArmorDamageRatio;
		invincibleTimer_ = Tuning::kArmorInvincibleTime;
	} else {
		// 吹っ飛び速度はダメージ閾値未満にクランプする（被弾中は低速帯）
		hp_ -= damage;
		const auto speed = std::min(Tuning::kKnockbackSpeed, Tuning::kDamageSpeed * 0.95f);
		const auto direction = knockbackDirection.Normalize();
		const auto velocityBefore = motion_.velocity;
		motion_.velocity = direction * speed;
		invincibleTimer_ = Tuning::kHitInvincibleTime;
		if (visual_) { visual_->OnImpact(direction, motion_.velocity - velocityBefore); }
	}

	hp_ = std::max(hp_, 0.0f);
	if (visual_) { visual_->OnHit(); }
	return true;
}


void ou::Player::AddStock(int count) {
	stock_ = std::min(stock_ + count, Tuning::kMaxStock);
}


void ou::Player::Bounce(const Vector2 &normal, float restitution) {
	const auto velocityBefore = motion_.velocity;
	motion_.velocity = Motion2D::Reflect(motion_.velocity, normal) * restitution;
	if (visual_) { visual_->OnImpact(normal, motion_.velocity - velocityBefore); }
}


void ou::Player::Freeze() {
	isFrozen_ = true;
	pull_.Cancel();
	TimeControl::SetSlowMotion(false);
}


void ou::Player::Launch() {
	motion_.velocity = ComputeLaunchVelocity();
	stock_--;
	stockTimer_ = 0.0f;
}
