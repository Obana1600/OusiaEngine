#include "Player.h"
#include "Core/Time.h"
#include "GameAudio.h"
#include "GameColors.h"
#include "ParticleEffects.h"
#include "Math/Mathf.h"
#include "PlayerVisual.h"
#include "Scene/GameObject.h"
#include "TimeControl.h"
#include "Tuning.h"

#include <algorithm>

using namespace ou;


namespace {

// 壁で跳ねた音を鳴らす、最小の衝撃（転がりや小さな跳ねでは鳴らさない）
constexpr float kBoundSoundImpact = 4.0f;

} // namespace



MotionParams ou::Player::MakeMotionParams() {
	MotionParams params;
	params.radius = Tuning::kPlayerRadius;
	params.drag = Tuning::kPlayerDrag;
	params.restitution = Tuning::kWallRestitution;
	params.gravity = Tuning::kGravity;
	return params;
}


void ou::Player::Awake() {
	sInstance_ = this;
	hp_ = Tuning::kPlayerMaxHp;
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

	// 自動突進の間は引っ張れない
	autoDash_.Update(*this, dt);
	if (autoDash_.IsActive()) {
		pull_.Cancel();
	} else {
		// 再発射は速さが再発射閾値以下のときだけ（回数の制限はない）
		const auto canStart = !IsDead() && CanRelaunch(GetBand());
		pull_.Update(Time::GetUnscaledDeltaTime(), canStart);
		if (pull_.IsFired()) { Launch(); }
	}
	TimeControl::SetSlowMotion(pull_.IsCharging());

	auto params = MakeMotionParams();
	if (!isGravityEnabled_) { params.gravity = 0.0f; }
	const auto result = Motion2D::Step(motion_, params, dt);
	if (result.bounceCount > 0) {
		if (visual_) { visual_->OnImpact(result.lastWallNormal, result.wallImpulse); }
		// 強く当たったときだけ、壁から火花を散らす
		const auto impact = result.wallImpulse.Length();
		// 転がっているだけの小さな跳ねでは鳴らさない。強いほど大きく鳴らす
		if (impact >= kBoundSoundImpact) {
			GameAudio::PlaySe(Se::PlayerBound, std::min(0.3f + impact / Tuning::kPlayerMaxSpeed, 1.0f));
		}
		if (impact >= Tuning::kDamageSpeed) {
			const auto contact = motion_.position - result.lastWallNormal * Tuning::kPlayerRadius;
			ParticleEffects::PlaySparks(contact, result.lastWallNormal, GameColors::ForBand(GetBand()), static_cast<int>(6.0f + impact * 0.3f));
		}
	}

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
	// 中速以上と、自動突進の溜め中は無敵
	if (IsInvincible() || IsDead() || isFrozen_ || IsInvincibleBySpeed(GetBand()) || autoDash_.IsWindingUp()) { return false; }

	// 吹っ飛び速度はダメージ閾値未満にクランプする（被弾中は低速帯）
	hp_ = std::max(hp_ - damage, 0.0f);
	const auto speed = std::min(Tuning::kKnockbackSpeed, Tuning::kDamageSpeed * 0.95f);
	const auto direction = knockbackDirection.Normalized();
	const auto velocityBefore = motion_.velocity;
	motion_.velocity = direction * speed;
	invincibleTimer_ = Tuning::kHitInvincibleTime;
	if (visual_) { visual_->OnImpact(direction, motion_.velocity - velocityBefore); }

	if (visual_) { visual_->OnHit(); }
	ParticleEffects::PlayExplosion(motion_.position, GameColors::kPlayerHit, 0.3f);
	GameAudio::PlaySe(Se::PlayerTakeDamage);
	return true;
}


void ou::Player::RequestAutoDash() {
	if (IsDead() || isFrozen_) { return; }
	autoDash_.Request();
}


void ou::Player::OnBossHit() {
	autoDash_.OnBossHit(*this);
}


void ou::Player::Bounce(const Vector2 &normal, float restitution) {
	const auto velocityBefore = motion_.velocity;
	motion_.velocity = Motion2D::Reflect(motion_.velocity, normal) * restitution;
	if (visual_) { visual_->OnImpact(normal, motion_.velocity - velocityBefore); }
}


void ou::Player::Freeze() {
	isFrozen_ = true;
	pull_.Cancel();
	autoDash_.Cancel(*this);
	TimeControl::SetSlowMotion(false);
}


void ou::Player::Launch() {
	motion_.velocity = ComputeLaunchVelocity();
	// 発射した反対側へ、溜めた分だけ火花を噴く
	const auto ratio = pull_.GetChargeRatio();
	ParticleEffects::PlayHit(motion_.position, -motion_.velocity.Normalized(), GameColors::ForBand(GetBand()), 0.2f + 0.6f * ratio);
	ParticleEffects::PlayRing(motion_.position, GameColors::ForBand(GetBand()), 1.0f + 1.5f * ratio);
	GameAudio::PlaySe(Se::PlayerDash, 0.5f + 0.5f * ratio);
}
