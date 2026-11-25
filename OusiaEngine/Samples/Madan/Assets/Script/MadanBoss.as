// 魔弾（仮）: ボス
// 体力 7。子の GameObject に付けた WeakPoint を、段階ごとに決まった流れで露出させる
//
// 1 段階の流れ（弱点に当てるまで繰り返す）
//   攻撃 (attackTime) → 予兆 (warningTime) → 露出 (exposeTime) → 攻撃 …
// 露出中の弱点に当てると、ひるみ (stunTime) のあと次の段階へ進む
enum MadanBossPhase {
	Waiting,  // 戦闘開始前
	Attack,   // 攻撃中
	Warning,  // 弱点が開く前の予兆
	Expose,   // 弱点が露出している
	Stun,     // 弱点に当たってひるんでいる
	Dead      // 倒された
}

enum MadanChargeStep {
	None,
	Windup,   // ため
	Dash      // 突進
}

class MadanBoss : MonoBehavior {
	// ---- 体 ----
	int maxHp = 7;
	// 体の当たり判定の半径。ここに当たった弾は（貫通弾以外）止まる
	float bodyRadius = 1.6f;
	// プレイヤーの方を向く速さ（度/秒）
	[SerializeField] private float turnSpeed = 90.0f;
	// 露出中もプレイヤーの方を向き続けるか。false なら背中の弱点を狙いに回り込める
	[SerializeField] private bool turnWhileExposed = false;
	// 体当たりのダメージを突進中だけにするか（false なら常に触れるとダメージ）
	[SerializeField] private bool contactDamageOnlyWhileCharging = true;

	// ---- 段階ごとの流れ（テストプレイで調整する数値） ----
	[SerializeField] private float attackTime = 4.0f;
	[SerializeField] private float warningTime = 0.6f;
	[SerializeField] private float exposeTime = 1.6f;
	[SerializeField] private float stunTime = 1.2f;
	// 露出中も攻撃を続けるか
	[SerializeField] private bool attackWhileExposed = false;
	// 弱点に 1 発当たるごとに攻撃の間隔が縮む割合（0.1 で 10% ずつ速くなる）
	[SerializeField] private float speedUpPerHit = 0.08f;

	// 段階ごとの攻撃。段階を "/"、同時に使う攻撃を "+" で区切る
	// 使える攻撃: Radial（全方位）, Aimed（自機狙い）, Spiral（渦巻き）, Charge（突進）
	// 段階が足りないときは最後の段階を使い回す
	[SerializeField] private string attackPlan = "Radial / Aimed / Radial+Aimed / Spiral / Charge+Radial / Spiral+Aimed / Charge+Spiral+Aimed";

	// ---- 弾 ----
	// 弾の見た目（Sphere を小さくしたものなど）。BossProjectile は自動で付く
	GameObject@ projectilePrefab;
	[SerializeField] private float projectileSpeed = 7.0f;
	[SerializeField] private float projectileRadius = 0.3f;
	[SerializeField] private float projectileLifeTime = 6.0f;
	[SerializeField] private float projectileHeight = 0.8f;

	// ---- 攻撃ごとの数値 ----
	[SerializeField] private int radialCount = 14;
	[SerializeField] private float radialInterval = 1.3f;
	[SerializeField] private int aimedCount = 3;
	[SerializeField] private float aimedSpread = 14.0f;
	[SerializeField] private float aimedInterval = 0.9f;
	[SerializeField] private int spiralArms = 2;
	[SerializeField] private float spiralInterval = 0.1f;
	[SerializeField] private float spiralTurnSpeed = 140.0f;
	[SerializeField] private float chargeWindupTime = 0.7f;
	[SerializeField] private float chargeSpeed = 15.0f;
	[SerializeField] private float chargeTime = 0.6f;
	// 予兆中に元の位置へ戻る速さ
	[SerializeField] private float returnSpeed = 6.0f;

	int hp = 7;
	MadanBossPhase phase = MadanBossPhase::Waiting;
	private float phaseTimer = 0.0f;
	private int stage = 0;
	private array<WeakPoint@> weakPoints;
	private array<WeakPoint@> currentWeakPoints;
	private array<string> stagePlans;
	private Vector3 homePosition;

	// 今の段階で使う攻撃
	private bool useRadial = false;
	private bool useAimed = false;
	private bool useSpiral = false;
	private bool useCharge = false;
	private float radialTimer = 0.0f;
	private float aimedTimer = 0.0f;
	private float spiralTimer = 0.0f;
	private float spiralAngle = 0.0f;
	private float radialOffset = 0.0f;
	private MadanChargeStep chargeStep = MadanChargeStep::None;
	private float chargeTimer = 0.0f;
	private bool chargeDone = false;
	private Vector3 chargeDirection = Vector3::forward;
	private bool warnedNoPrefab = false;

	bool get_isDead() const property { return hp <= 0; }
	int get_currentStage() const property { return stage; }
	bool get_isCharging() const property { return chargeStep == MadanChargeStep::Dash; }

	void Awake() {
		@Madan::boss = this;
		hp = maxHp;
		homePosition = transform.position;
		stagePlans = Madan::Split(attackPlan, "/");
	}

	void OnDestroy() {
		if (Madan::boss is this) { @Madan::boss = null; }
	}

	void Start() {
		weakPoints.resize(0);
		CollectWeakPoints(transform);
		if (weakPoints.isEmpty()) { Debug::LogError("MadanBoss: 子に WeakPoint がありません"); }
		for (uint i = 0; i < weakPoints.length(); i++) { weakPoints[i].SetOpen(false); }
	}

	void Update() {
		const float dt = Time::deltaTime;

		if (phase == MadanBossPhase::Dead) { return; }
		if (phase == MadanBossPhase::Waiting) {
			if (Madan::IsFighting()) { EnterStage(0); }
			return;
		}
		if (!Madan::IsFighting()) { return; }

		phaseTimer += dt;
		UpdateFacing(dt);
		CheckContact();

		switch (phase) {
		case MadanBossPhase::Attack:
			UpdateAttacks(dt);
			if (phaseTimer >= attackTime && chargeStep == MadanChargeStep::None) { SetPhase(MadanBossPhase::Warning); }
			break;
		case MadanBossPhase::Warning:
			ReturnHome(dt);
			if (phaseTimer >= warningTime) { SetPhase(MadanBossPhase::Expose); }
			break;
		case MadanBossPhase::Expose:
			if (attackWhileExposed) { UpdateAttacks(dt); }
			if (phaseTimer >= exposeTime) { SetPhase(MadanBossPhase::Attack); }
			break;
		case MadanBossPhase::Stun:
			ReturnHome(dt);
			if (phaseTimer >= stunTime) { EnterStage(maxHp - hp); }
			break;
		}
	}

	// ---- 弾から呼ばれる ----

	// 露出中の弱点のうち、position からいちばん近いもの（range 以内）
	WeakPoint@ FindOpenWeakPoint(const Vector3&in position, float range) const {
		WeakPoint@ best = null;
		float bestDistance = range;
		for (uint i = 0; i < currentWeakPoints.length(); i++) {
			WeakPoint@ wp = currentWeakPoints[i];
			if (!wp.isOpen) { continue; }
			const float d = Madan::FlatDistance(wp.center, position);
			if (d <= bestDistance) { bestDistance = d; @best = wp; }
		}
		return best;
	}

	// 露出中の弱点のうち、狙った向きにいちばん近いもの（距離は問わない）
	WeakPoint@ FindOpenWeakPointByAim(const Vector3&in position, const Vector3&in direction) const {
		WeakPoint@ best = null;
		float bestAngle = 1000.0f;
		for (uint i = 0; i < currentWeakPoints.length(); i++) {
			WeakPoint@ wp = currentWeakPoints[i];
			if (!wp.isOpen) { continue; }
			const float angle = Vector3::Angle(direction, Madan::Flat(wp.center - position));
			if (angle < bestAngle) { bestAngle = angle; @best = wp; }
		}
		return best;
	}

	// 線分 from→to が露出中の弱点に当たるか
	WeakPoint@ HitTestWeakPoint(const Vector3&in from, const Vector3&in to, float radius) const {
		if (phase != MadanBossPhase::Expose) { return null; }
		for (uint i = 0; i < currentWeakPoints.length(); i++) {
			WeakPoint@ wp = currentWeakPoints[i];
			if (wp.isOpen && Madan::SegmentPointDistance(from, to, wp.center) <= radius + wp.radius) { return wp; }
		}
		return null;
	}

	// 線分 from→to がボスの体に当たるか
	bool BlocksBullet(const Vector3&in from, const Vector3&in to, float radius) const {
		return Madan::SegmentPointDistance(from, to, transform.position) <= bodyRadius + radius;
	}

	bool IsCurrentWeakPoint(WeakPoint@ wp) const {
		return phase != MadanBossPhase::Stun && currentWeakPoints.findByRef(wp) >= 0;
	}

	// 弱点に当たった。force は魔王の弾（閉じかけでも当たる）
	void OnWeakPointHit(WeakPoint@ wp, MadanBulletType bulletType, bool force) {
		if (isDead || phase == MadanBossPhase::Stun) { return; }
		if (!IsCurrentWeakPoint(wp)) { return; }
		if (!force && !wp.isOpen) { return; }

		hp--;
		Debug::Log("[魔弾] 弱点に命中（" + Madan::BulletLabel(bulletType) + "）ボスの残り体力 " + hp);

		for (uint i = 0; i < currentWeakPoints.length(); i++) {
			WeakPoint@ point = currentWeakPoints[i];
			point.SetOpen(false);
			if (point.lastStage <= stage) { point.MarkBroken(); }
		}
		chargeStep = MadanChargeStep::None;

		if (hp <= 0) {
			SetPhase(MadanBossPhase::Dead);
			Debug::Log("[魔弾] ボスを倒した");
			return;
		}
		SetPhase(MadanBossPhase::Stun);
	}

	// ---- 段階と状態 ----
	private void EnterStage(int index) {
		stage = index;
		currentWeakPoints.resize(0);
		for (uint i = 0; i < weakPoints.length(); i++) {
			if (weakPoints[i].HasStage(stage)) { currentWeakPoints.insertLast(weakPoints[i]); }
		}
		if (currentWeakPoints.isEmpty()) {
			Debug::LogWarning("MadanBoss: 段階 " + stage + " の WeakPoint がありません。すべての弱点を使います");
			for (uint i = 0; i < weakPoints.length(); i++) { currentWeakPoints.insertLast(weakPoints[i]); }
		}

		string plan = stagePlans.isEmpty() ? "Radial" : stagePlans[uint(Mathf::Min(stage, int(stagePlans.length()) - 1))];
		useRadial = plan.findFirst("Radial") >= 0;
		useAimed = plan.findFirst("Aimed") >= 0;
		useSpiral = plan.findFirst("Spiral") >= 0;
		useCharge = plan.findFirst("Charge") >= 0;

		SetPhase(MadanBossPhase::Attack);
	}

	private void SetPhase(MadanBossPhase next) {
		phase = next;
		phaseTimer = 0.0f;

		const bool warning = next == MadanBossPhase::Warning;
		const bool open = next == MadanBossPhase::Expose;
		if (next != MadanBossPhase::Dead) {
			for (uint i = 0; i < currentWeakPoints.length(); i++) {
				currentWeakPoints[i].SetOpen(open);
				currentWeakPoints[i].SetWarning(warning);
			}
		}

		if (next == MadanBossPhase::Attack) {
			radialTimer = 0.3f;
			aimedTimer = 0.6f;
			spiralTimer = 0.0f;
			chargeDone = false;
			chargeStep = MadanChargeStep::None;
		}
	}

	private void CollectWeakPoints(Transform@ parent) {
		for (int i = 0; i < parent.childCount; i++) {
			Transform@ child = parent.GetChild(i);
			WeakPoint@ wp = child.GetComponent<WeakPoint>();
			if (wp !is null) { weakPoints.insertLast(wp); }
			CollectWeakPoints(child);
		}
	}

	// ---- 動き ----
	private void UpdateFacing(float dt) {
		if (phase == MadanBossPhase::Stun) { return; }
		if (phase == MadanBossPhase::Expose && !turnWhileExposed) { return; }
		if (chargeStep == MadanChargeStep::Dash) { return; }
		MadanPlayer@ player = Madan::player;
		if (player is null) { return; }

		const Vector3 toPlayer = Madan::Flat(player.transform.position - transform.position);
		if (toPlayer.sqrMagnitude < 0.0001f) { return; }
		const Quaternion want = Quaternion::LookRotation(toPlayer.normalized);
		const float angle = Quaternion::Angle(transform.rotation, want);
		if (angle < 0.01f) { return; }
		const float t = Mathf::Min(1.0f, turnSpeed * dt / angle);
		transform.rotation = Quaternion::Slerp(transform.rotation, want, t);
	}

	private void ReturnHome(float dt) {
		transform.position = Vector3::MoveTowards(transform.position, homePosition, returnSpeed * dt);
	}

	private void CheckContact() {
		MadanPlayer@ player = Madan::player;
		if (player is null || player.isDead) { return; }
		if (contactDamageOnlyWhileCharging && chargeStep != MadanChargeStep::Dash) { return; }
		const float distance = Madan::FlatDistance(player.transform.position, transform.position);
		if (distance <= bodyRadius + player.hitRadius + 0.05f) { player.TakeHit(); }
	}

	// ---- 攻撃 ----
	private float get_speedRate() const property {
		return 1.0f + float(maxHp - hp) * speedUpPerHit;
	}

	private void UpdateAttacks(float dt) {
		const float rate = speedRate;

		if (useCharge && UpdateCharge(dt)) { return; } // 突進中は弾を撃たない

		if (useRadial) {
			radialTimer -= dt;
			if (radialTimer <= 0.0f) {
				radialTimer += radialInterval / rate;
				FireRadial();
			}
		}
		if (useAimed) {
			aimedTimer -= dt;
			if (aimedTimer <= 0.0f) {
				aimedTimer += aimedInterval / rate;
				FireAimed();
			}
		}
		if (useSpiral) {
			spiralAngle += spiralTurnSpeed * rate * dt;
			spiralTimer -= dt;
			if (spiralTimer <= 0.0f) {
				spiralTimer += spiralInterval / rate;
				FireSpiral();
			}
		}
	}

	// 突進の処理。突進中（ため含む）なら true
	private bool UpdateCharge(float dt) {
		switch (chargeStep) {
		case MadanChargeStep::None:
			if (chargeDone || phase != MadanBossPhase::Attack) { return false; }
			chargeStep = MadanChargeStep::Windup;
			chargeTimer = 0.0f;
			return true;
		case MadanChargeStep::Windup: {
			chargeTimer += dt;
			// ためている間は小刻みに震える
			const float shake = Mathf::Sin(Time::time * 60.0f) * 0.06f;
			transform.position = transform.position + transform.right * shake;
			if (chargeTimer >= chargeWindupTime) {
				MadanPlayer@ player = Madan::player;
				chargeDirection = player is null ? Madan::Flat(transform.forward) : Madan::Flat(player.transform.position - transform.position);
				if (chargeDirection.sqrMagnitude < 0.0001f) { chargeDirection = Madan::Flat(transform.forward); }
				chargeDirection = chargeDirection.normalized;
				transform.rotation = Quaternion::LookRotation(chargeDirection);
				chargeStep = MadanChargeStep::Dash;
				chargeTimer = 0.0f;
			}
			return true;
		}
		case MadanChargeStep::Dash: {
			chargeTimer += dt;
			Vector3 position = transform.position + chargeDirection * chargeSpeed * dt;
			if (Madan::battle !is null) { position = Madan::battle.ClampToArena(position, bodyRadius); }
			transform.position = position;
			if (chargeTimer >= chargeTime) {
				chargeStep = MadanChargeStep::None;
				chargeDone = true;
			}
			return true;
		}
		}
		return false;
	}

	private void FireRadial() {
		const int count = Mathf::Max(1, radialCount);
		const float step = 360.0f / float(count);
		radialOffset += step * 0.5f; // 毎回少しずらして同じ安全地帯にしない
		for (int i = 0; i < count; i++) {
			Shoot(Madan::RotateY(Vector3::forward, radialOffset + step * float(i)));
		}
	}

	private void FireAimed() {
		MadanPlayer@ player = Madan::player;
		if (player is null) { return; }
		Vector3 dir = Madan::Flat(player.transform.position - transform.position);
		if (dir.sqrMagnitude < 0.0001f) { return; }
		dir = dir.normalized;
		const int count = Mathf::Max(1, aimedCount);
		const float start = -aimedSpread * float(count - 1) * 0.5f;
		for (int i = 0; i < count; i++) {
			Shoot(Madan::RotateY(dir, start + aimedSpread * float(i)));
		}
	}

	private void FireSpiral() {
		const int arms = Mathf::Max(1, spiralArms);
		for (int i = 0; i < arms; i++) {
			Shoot(Madan::RotateY(Vector3::forward, spiralAngle + 360.0f * float(i) / float(arms)));
		}
	}

	private void Shoot(const Vector3&in direction) {
		const Vector3 dir = Madan::Flat(direction).normalized;
		const Vector3 origin = Madan::Flat(transform.position) + dir * bodyRadius + Vector3::up * projectileHeight;

		GameObject@ go;
		if (projectilePrefab !is null) {
			@go = Instantiate(projectilePrefab, origin, Quaternion::LookRotation(dir));
		} else {
			@go = GameObject("BossProjectile");
			go.transform.position = origin;
			if (!warnedNoPrefab) {
				Debug::LogWarning("MadanBoss: projectilePrefab が未設定のため、弾が見えません");
				warnedNoPrefab = true;
			}
		}

		BossProjectile@ projectile = go.GetComponent<BossProjectile>();
		if (projectile is null) { @projectile = go.AddComponent<BossProjectile>(); }
		if (projectile !is null) { projectile.Launch(dir * projectileSpeed, projectileRadius, projectileLifeTime); }
	}

	void OnDrawGizmosSelected() {
		Gizmos::color = Color::cyan;
		Gizmos::DrawWireSphere(transform.position, bodyRadius);
	}
}
