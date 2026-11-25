// 魔弾（仮）: ボスの撃つ弾
// MadanBoss が弾の見た目に自動で付けて Launch を呼ぶ。プレイヤーに当たると消える
class BossProjectile : MonoBehavior {
	private Vector3 velocity = Vector3::zero;
	private float radius = 0.3f;
	private float lifeTime = 6.0f;
	private float age = 0.0f;
	private bool launched = false;

	void Launch(const Vector3&in initialVelocity, float hitRadius, float life) {
		velocity = Madan::Flat(initialVelocity);
		radius = hitRadius;
		lifeTime = life;
		launched = true;
		if (velocity.sqrMagnitude > 0.0001f) { transform.rotation = Quaternion::LookRotation(velocity); }
	}

	void Update() {
		if (!launched) { return; }
		const float dt = Time::deltaTime;
		age += dt;

		// 決着がついたら弾は消す
		if (Madan::battle !is null && (Madan::battle.state == MadanBattleState::Won || Madan::battle.state == MadanBattleState::Lost)) {
			Destroy(gameObject);
			launched = false;
			return;
		}

		const Vector3 from = transform.position;
		const Vector3 to = from + velocity * dt;
		transform.position = to;

		MadanPlayer@ player = Madan::player;
		if (player !is null && !player.isDead &&
			Madan::SegmentPointDistance(from, to, player.transform.position) <= radius + player.hitRadius) {
			if (player.TakeHit()) {
				Destroy(gameObject);
				launched = false;
				return;
			}
		}

		const bool outside = Madan::battle !is null && Madan::battle.IsOutsideArena(to, 2.0f);
		if (outside || age >= lifeTime) {
			Destroy(gameObject);
			launched = false;
		}
	}
}
