#include "CombatSystem.h"
#include "Boss.h"
#include "CameraRig.h"
#include "ComboCounter.h"
#include "GameColors.h"
#include "GameAudio.h"
#include "Math/Mathf.h"
#include "ParticleEffects.h"
#include "Player.h"
#include "Projectile.h"
#include "TimeControl.h"
#include "Tuning.h"

#include <vector>

using namespace ou;


namespace {

// ダメージ速度から最大速度までのこの割合以上で当てたら、最大溜めの音を鳴らす
constexpr float kMaxChargeHitRatio = 0.85f;

// 2 つの円が重なっていれば、a から見た b の向き（法線）と重なりの深さを返す
bool OverlapCircles(const Vector2 &a, float radiusA, const Vector2 &b, float radiusB, Vector2 *normal, float *depth) {
	const auto offset = b - a;
	const auto distance = offset.Length();
	const auto radiusSum = radiusA + radiusB;
	if (distance >= radiusSum) { return false; }
	*normal = distance > 0.0001f ? offset / distance : Vector2{0.0f, 1.0f};
	*depth = radiusSum - distance;
	return true;
}

} // namespace



void ou::CombatSystem::Update() {
	auto *player = Player::GetInstance();
	if (!player || player->IsDead()) { return; }

	if (auto *boss = Boss::GetInstance(); boss && boss->GetState() != BossState::Defeated) { ResolvePlayerBoss(*player, *boss); }

	// 判定の途中で弾が消えるので、写してから回す
	const auto projectiles = Projectile::GetAll();
	for (auto *projectile : projectiles) { ResolvePlayerProjectile(*player, *projectile); }
}


void ou::CombatSystem::ResolvePlayerBoss(Player &player, Boss &boss) {
	Vector2 normal;
	float depth = 0.0f;
	// normal はボスからプレイヤーへの向き
	if (!OverlapCircles(boss.GetPosition(), boss.GetRadius(), player.GetPosition(), player.GetRadius(), &normal, &depth)) { return; }

	// 重なりを解消する
	player.SetPosition(player.GetPosition() + normal * depth);

	const auto band = player.GetBand();
	const auto speed = player.GetSpeed();
	const auto contact = boss.GetPosition() + normal * boss.GetRadius();
	// 弱点に当たったら金色に光らせる
	const auto color = boss.IsWeakPointHit(normal) ? GameColors::kGold : GameColors::ForBand(band);

	if (boss.IsDashing()) {
		if (band == SpeedBand::High) {
			// 突進中のボスと高速のプレイヤー：両方反射する。プレイヤーは速さによる無敵なので、ダメージはボスだけ
			boss.TakeDamage(speed * Tuning::kDamagePerSpeed, normal);
			boss.ReflectDash(-normal);
			player.Bounce(normal, Tuning::kBossBounceRestitution);
			player.OnBossHit();
			OnScore(speed, contact, normal, color);
			ParticleEffects::PlayExplosion(contact, GameColors::kBossDanger, 0.8f);
		} else {
			// 低速なら吹っ飛ぶ。中速は無敵なので押し返されるだけ
			if (!player.TakeHit(Tuning::kDashDamage, normal)) { player.Bounce(normal, 1.0f); }
		}
		return;
	}

	if (HasAttack(band)) {
		// 中速以上の体当たり。ダメージは速さに比例し、プレイヤーはボスから跳ね返る
		if (boss.TakeDamage(speed * Tuning::kDamagePerSpeed, normal) > 0.0f) { OnScore(speed, contact, normal, color); }
		player.Bounce(normal, Tuning::kBossBounceRestitution);
		player.OnBossHit();
		return;
	}

	// 低速で触れたときは押し返されるだけ
	player.Bounce(normal, 1.0f);
}


void ou::CombatSystem::ResolvePlayerProjectile(Player &player, Projectile &projectile) {
	Vector2 normal;
	float depth = 0.0f;
	// normal は弾からプレイヤーへの向き
	if (!OverlapCircles(projectile.GetPosition(), projectile.GetRadius(), player.GetPosition(), player.GetRadius(), &normal, &depth)) { return; }

	if (projectile.GetKind() == ProjectileKind::Giant) {
		if (player.GetBand() == SpeedBand::High) {
			// 高速なら巨大弾を壊して跳ね返る。ダメージは受けず、コンボに加算する
			const auto speed = player.GetSpeed();
			player.SetPosition(player.GetPosition() + normal * depth);
			player.Bounce(normal, Tuning::kBossBounceRestitution);
			ParticleEffects::PlayExplosion(projectile.GetPosition(), GameColors::kGiantBullet, projectile.GetRadius());
			projectile.Vanish();
			OnScore(speed, projectile.GetPosition() + normal * projectile.GetRadius(), normal, GameColors::kPlayerHigh);
			return;
		}
		// 高速でなければ通常どおり被弾する（巨大弾は残る）
		player.TakeHit(projectile.GetDamage(), normal);
		return;
	}

	// 小弾・追尾弾は触れると消える（無敵中は弾けるだけ）
	player.TakeHit(projectile.GetDamage(), normal);
	const auto color = projectile.GetKind() == ProjectileKind::Homing ? GameColors::kHomingBullet : GameColors::kSmallBullet;
	ParticleEffects::PlaySparks(projectile.GetPosition(), -normal, color, 8);
	projectile.Vanish();
}


void ou::CombatSystem::OnScore(float speed, const Vector2 &position, const Vector2 &direction, const Color &color) {
	if (auto *combo = ComboCounter::GetInstance()) { combo->Add(); }

	// 速いほど長く止め、派手にはじける
	const auto t = Mathf::InverseLerp(Tuning::kDamageSpeed, Tuning::kPlayerMaxSpeed, speed);
	// 最大近くまで溜めた一撃は、専用の音にする
	GameAudio::PlaySe(t >= kMaxChargeHitRatio ? Se::PlayerDealDamageMaxCharge : Se::PlayerDealDamage);
	TimeControl::RequestHitStop(Mathf::Lerp(Tuning::kHitStopMin, Tuning::kHitStopMax, t));
	if (auto *camera = CameraRig::GetInstance()) { camera->Shake(0.15f + 0.35f * t); }
	ParticleEffects::PlayHit(position, direction, color, 0.4f + 0.6f * t);
	ParticleEffects::PlayRing(position, color, 1.5f + 2.5f * t);
}
