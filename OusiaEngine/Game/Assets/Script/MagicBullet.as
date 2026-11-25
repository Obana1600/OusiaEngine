// 魔弾（仮）: 撃った魔弾 1 発
// MagicGun が弾の見た目に自動で付けて Launch を呼ぶ。数値は MagicGun 側にある

// 散弾の子弾どうしで共有する。どれか 1 発が当たれば残りは消える
class MadanShotGroup {
	bool resolved = false;
}

class MagicBullet : MonoBehavior {
	private MagicGun@ gun;
	private MadanBulletType type = MadanBulletType::Normal;
	private int slot = -1;
	private MadanShotGroup@ group;
	private Vector3 direction = Vector3::forward;
	private float speed = 20.0f;
	private float radius = 0.2f;
	private float age = 0.0f;
	private bool launched = false;
	private bool finished = false;
	private bool recalling = false;
	private WeakPoint@ devilTarget;
	private bool devilTargetsShooter = false;

	void Launch(MagicGun@ owner, MadanBulletType bulletType, int slotIndex, const Vector3&in dir, MadanShotGroup@ shotGroup) {
		@gun = owner;
		type = bulletType;
		slot = slotIndex;
		@group = shotGroup;
		direction = Madan::Flat(dir).normalized;
		speed = gun.bulletSpeed;
		radius = type == MadanBulletType::Large ? gun.largeRadius : gun.bulletRadius;
		if (type == MadanBulletType::Large && gun.bulletRadius > 0.0f) {
			transform.localScale = transform.localScale * (gun.largeRadius / gun.bulletRadius);
		}

		if (type == MadanBulletType::Devil) { ChooseDevilTarget(); }

		launched = true;
		if (Madan::battle !is null) { Madan::battle.RegisterBullet(); }
		FaceDirection();
	}

	void OnDestroy() {
		// Finish を通らずに消えた（シーンの切り替えなど）ときも数を戻す
		if (launched && !finished && Madan::battle !is null) { Madan::battle.UnregisterBullet(); }
		finished = true;
	}

	void Update() {
		if (!launched || finished) { return; }
		const float dt = Time::deltaTime;
		age += dt;

		// 散弾の別の子弾が当たった
		if (group !is null && group.resolved) { Finish(); return; }

		if (recalling) { UpdateRecall(dt); return; }

		Steer(dt);

		const Vector3 from = transform.position;
		const Vector3 to = from + direction * speed * dt;
		transform.position = to;
		FaceDirection();

		if (CheckHit(from, to)) { return; }

		const bool outside = Madan::battle !is null && Madan::battle.IsOutsideArena(to, 1.0f);
		if (outside || age >= gun.bulletLifeTime) { Miss(); }
	}

	// ---- 進む向き ----
	private void Steer(float dt) {
		Vector3 target;
		float turnSpeed = 0.0f;

		if (type == MadanBulletType::Devil) {
			if (devilTarget !is null && !IsTargetValid(devilTarget)) { @devilTarget = null; }
			if (devilTarget !is null) {
				target = devilTarget.center;
			} else if (devilTargetsShooter) {
				target = gun.shooterPosition;
			} else {
				return;
			}
			turnSpeed = gun.devilTurnSpeed;
			// 発射直後はプレイヤー自身に当たらないよう、少し飛んでから戻ってくる
			if (devilTargetsShooter && age < 0.35f) { return; }
		} else if (type == MadanBulletType::Homing) {
			MadanBoss@ boss = Madan::boss;
			if (boss is null) { return; }
			WeakPoint@ nearest = boss.FindOpenWeakPoint(transform.position, gun.homingRange);
			if (nearest is null) { return; }
			target = nearest.center;
			turnSpeed = gun.homingTurnSpeed;
		} else {
			return;
		}

		const Vector3 want = Madan::Flat(target - transform.position);
		if (want.sqrMagnitude < 0.0001f) { return; }
		direction = TurnTowards(direction, want.normalized, turnSpeed * dt);
	}

	// ---- 当たり判定 ----
	private bool CheckHit(const Vector3&in from, const Vector3&in to) {
		MadanBoss@ boss = Madan::boss;

		// 魔王の弾は狙った相手にだけ当たる
		if (type == MadanBulletType::Devil) {
			if (devilTarget !is null) {
				if (Madan::SegmentPointDistance(from, to, devilTarget.center) <= radius + devilTarget.radius) {
					HitWeakPoint(devilTarget, true);
					return true;
				}
				return false;
			}
			if (devilTargetsShooter) {
				MadanPlayer@ player = Madan::player;
				if (player !is null && age >= 0.35f &&
					Madan::SegmentPointDistance(from, to, player.transform.position) <= radius + player.hitRadius) {
					player.TakeHit(true);
					Finish();
					return true;
				}
				return false;
			}
		}

		if (boss is null || boss.isDead) { return false; }

		WeakPoint@ hit = boss.HitTestWeakPoint(from, to, radius);
		if (hit !is null) {
			HitWeakPoint(hit, false);
			return true;
		}

		// 貫通弾以外は、ボスの体に当たると止まる
		if (type != MadanBulletType::Pierce && boss.BlocksBullet(from, to, radius)) {
			Miss();
			return true;
		}
		return false;
	}

	private void HitWeakPoint(WeakPoint@ weakPoint, bool force) {
		if (group !is null) {
			if (group.resolved) { Finish(); return; }
			group.resolved = true;
		}
		MadanBoss@ boss = Madan::boss;
		if (boss !is null) { boss.OnWeakPointHit(weakPoint, type, force); }
		Finish();
	}

	// ---- 外れたとき ----
	private void Miss() {
		if (type == MadanBulletType::Recall && !recalling) {
			recalling = true;
			return;
		}
		Finish();
	}

	private void UpdateRecall(float dt) {
		const Vector3 home = gun.shooterPosition;
		Vector3 toHome = home - transform.position;
		const float step = gun.recallSpeed * dt;
		if (toHome.magnitude <= step + 0.3f) {
			gun.Refund(slot);
			Finish();
			return;
		}
		direction = toHome.normalized;
		transform.position = transform.position + direction * step;
		FaceDirection();
	}

	private void Finish() {
		if (finished) { return; }
		finished = true;
		if (Madan::battle !is null) { Madan::battle.UnregisterBullet(); }
		Destroy(gameObject);
	}

	// ---- 魔王の弾 ----
	private void ChooseDevilTarget() {
		MadanBoss@ boss = Madan::boss;
		@devilTarget = null;
		if (boss !is null) { @devilTarget = boss.FindOpenWeakPointByAim(transform.position, direction); }
		devilTargetsShooter = devilTarget is null && gun.devilHitsShooter;
		if (devilTarget !is null) {
			Debug::Log("[魔弾] 魔王の弾は弱点を捉えた");
		} else if (devilTargetsShooter) {
			Debug::Log("[魔弾] 魔王の弾は撃ち手を狙う…");
		}
	}

	private bool IsTargetValid(WeakPoint@ target) const {
		MadanBoss@ boss = Madan::boss;
		return boss !is null && !boss.isDead && boss.IsCurrentWeakPoint(target);
	}

	// ---- 補助 ----
	private void FaceDirection() {
		if (direction.sqrMagnitude > 0.0001f) { transform.rotation = Quaternion::LookRotation(direction); }
	}

	private Vector3 TurnTowards(const Vector3&in current, const Vector3&in want, float maxDegrees) const {
		const float angle = Vector3::Angle(current, want);
		if (angle <= maxDegrees || angle < 0.01f) { return want; }
		const float sign = Vector3::Cross(current, want).y >= 0.0f ? 1.0f : -1.0f;
		return Madan::RotateY(current, maxDegrees * sign).normalized;
	}
}
