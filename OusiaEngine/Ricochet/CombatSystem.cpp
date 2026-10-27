#include "CombatSystem.h"
#include "Boss.h"
#include "CameraRig.h"
#include "ComboCounter.h"
#include "Math/Mathf.h"
#include "Player.h"
#include "Projectile.h"
#include "TimeControl.h"
#include "Tuning.h"

#include <vector>

using namespace ou;


namespace {

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

	if (boss.IsDashing()) {
		if (band == SpeedBand::High) {
			// 突進中のボスと高速のプレイヤー：両方ダメージを受け、両方反射する
			boss.TakeDamage(speed * Tuning::kDamagePerSpeed, normal);
			boss.ReflectDash(-normal);
			player.Bounce(normal, Tuning::kBossBounceRestitution);
			player.TakeHit(Tuning::kDashDamage * 0.5f, normal);
			OnScore(speed);
		} else {
			player.TakeHit(Tuning::kDashDamage, normal);
		}
		return;
	}

	if (HasAttack(band)) {
		// 中速以上の体当たり。ダメージは速さに比例し、プレイヤーはボスから跳ね返る
		if (boss.TakeDamage(speed * Tuning::kDamagePerSpeed, normal) > 0.0f) { OnScore(speed); }
		player.Bounce(normal, Tuning::kBossBounceRestitution);
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
			projectile.Vanish();
			OnScore(speed);
			return;
		}
		// 高速でなければ通常どおり被弾する（巨大弾は残る）
		player.TakeHit(projectile.GetDamage(), normal);
		return;
	}

	// 小弾・追尾弾は触れると消える
	player.TakeHit(projectile.GetDamage(), normal);
	projectile.Vanish();
}


void ou::CombatSystem::OnScore(float speed) {
	if (auto *combo = ComboCounter::GetInstance()) { combo->Add(); }

	// 速いほど長く止める
	const auto t = Mathf::InverseLerp(Tuning::kDamageSpeed, Tuning::kPlayerMaxSpeed, speed);
	TimeControl::RequestHitStop(Mathf::Lerp(Tuning::kHitStopMin, Tuning::kHitStopMax, t));
	if (auto *camera = CameraRig::GetInstance()) { camera->Shake(0.15f + 0.35f * t); }
}
