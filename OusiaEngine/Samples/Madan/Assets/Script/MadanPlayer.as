// 魔弾（仮）: プレイヤー
// 移動・回避・照準・被弾を受け持つ。弾の管理は同じ GameObject の MagicGun が行う
//
// 操作
//   移動     : WASD（カメラの向きが基準）
//   狙う     : マウスカーソル（常にカーソルの方を向く）
//   撃つ     : 左クリック
//   回避     : Space / 左 Shift
//   弾を選ぶ : Q / E（MagicGun.allowSelect が true のときだけ）
class MadanPlayer : MonoBehavior {
	// ---- 移動 ----
	[SerializeField] private float moveSpeed = 6.0f;

	// ---- 回避 ----
	[SerializeField] private float dodgeSpeed = 16.0f;
	[SerializeField] private float dodgeTime = 0.18f;
	[SerializeField] private float dodgeCooldown = 0.45f;
	// 回避中は無敵にするか
	[SerializeField] private bool dodgeInvincible = true;

	// ---- 被弾 ----
	// 体力。1 なら一撃で死亡
	int maxHp = 1;
	// 被弾したあとの無敵時間（秒）。maxHp が 2 以上のときに意味を持つ
	[SerializeField] private float invincibleTimeAfterHit = 1.0f;
	// 当たり判定の半径
	float hitRadius = 0.35f;

	// ---- 照準 ----
	// カーソルの位置に置く照準（任意）。床に置く輪っかなどを割り当てる
	Transform@ aimMarker;
	// 照準を床から浮かせる高さ
	[SerializeField] private float aimMarkerHeight = 0.05f;

	int hp = 1;
	private Vector3 facing = Vector3::forward;
	private float dodgeTimer = 0.0f;
	private float dodgeCooldownTimer = 0.0f;
	private float invincibleTimer = 0.0f;
	private Vector3 dodgeDirection = Vector3::forward;
	private MagicGun@ gun;
	private MeshRenderer@ bodyRenderer;

	bool get_isDead() const property { return hp <= 0; }
	bool get_isDodging() const property { return dodgeTimer > 0.0f; }
	Vector3 get_aimDirection() const property { return facing; }

	void Awake() {
		@Madan::player = this;
		hp = maxHp;
		@gun = gameObject.GetComponent<MagicGun>();
		@bodyRenderer = gameObject.GetComponent<MeshRenderer>();
		if (gun is null) { Debug::LogWarning("MadanPlayer: 同じ GameObject に MagicGun を付けてください"); }
		facing = Madan::Flat(transform.forward);
		if (facing.sqrMagnitude < 0.0001f) { facing = Vector3::forward; }
		facing = facing.normalized;
		SetMarkerVisible(false);
	}

	void OnDestroy() {
		if (Madan::player is this) { @Madan::player = null; }
	}

	void Update() {
		const float dt = Time::deltaTime;
		dodgeCooldownTimer -= dt;
		invincibleTimer -= dt;
		UpdateBlink();

		if (isDead || !Madan::IsFighting()) {
			SetMarkerVisible(false);
			return;
		}

		// 照準は回避中も動かす
		UpdateAim();

		// 回避中は入力を受け付けずに流れる
		if (dodgeTimer > 0.0f) {
			dodgeTimer -= dt;
			Move(dodgeDirection * dodgeSpeed * dt);
			return;
		}

		const Vector3 input = ReadMoveInput();

		if (DodgePressed() && dodgeCooldownTimer <= 0.0f) {
			dodgeDirection = input.sqrMagnitude > 0.01f ? input.normalized : facing;
			dodgeTimer = dodgeTime;
			dodgeCooldownTimer = dodgeTime + dodgeCooldown;
			return;
		}

		if (gun !is null) {
			if (Input::GetKeyDown(KeyCode::Q)) { gun.SelectPrevious(); }
			if (Input::GetKeyDown(KeyCode::E)) { gun.SelectNext(); }
			if (Input::GetMouseButtonDown(0)) { gun.Fire(facing); }
		}

		Move(input * moveSpeed * dt);
	}

	// 攻撃を受けたときに呼ぶ。ダメージになったら true
	// ignoreInvincible: 回避や無敵時間を無視する（魔王の弾など）
	bool TakeHit(bool ignoreInvincible = false) {
		if (isDead) { return false; }
		if (!ignoreInvincible) {
			if (invincibleTimer > 0.0f) { return false; }
			if (dodgeInvincible && dodgeTimer > 0.0f) { return false; }
		}

		hp--;
		invincibleTimer = invincibleTimeAfterHit;
		Debug::Log("[魔弾] プレイヤー被弾 残り体力 " + hp);
		if (isDead) {
			SetMarkerVisible(false);
			if (bodyRenderer !is null) { bodyRenderer.enabled = true; }
		}
		return true;
	}

	// カーソルの方を向き、照準をカーソルの位置に置く
	private void UpdateAim() {
		Vector3 cursor;
		if (!Madan::TryGetCursorPoint(transform.position.y, cursor)) {
			SetMarkerVisible(false);
			return;
		}

		const Vector3 toCursor = Madan::Flat(cursor - transform.position);
		if (toCursor.sqrMagnitude > 0.0001f) { facing = toCursor.normalized; }
		transform.rotation = Quaternion::LookRotation(facing);

		SetMarkerVisible(true);
		if (aimMarker !is null) { aimMarker.position = cursor + Vector3::up * aimMarkerHeight; }
	}

	// WASD をカメラの向きに合わせた XZ 平面の向きにする
	private Vector3 ReadMoveInput() const {
		float x = 0.0f;
		float z = 0.0f;
		if (Input::GetKey(KeyCode::D)) { x += 1.0f; }
		if (Input::GetKey(KeyCode::A)) { x -= 1.0f; }
		if (Input::GetKey(KeyCode::W)) { z += 1.0f; }
		if (Input::GetKey(KeyCode::S)) { z -= 1.0f; }

		Vector3 forward;
		Vector3 right;
		Madan::GetCameraAxes(forward, right);
		Vector3 input = right * x + forward * z;
		if (input.sqrMagnitude > 1.0f) { input = input.normalized; }
		return input;
	}

	private bool DodgePressed() const {
		return Input::GetKeyDown(KeyCode::Space) || Input::GetKeyDown(KeyCode::LeftShift);
	}

	// アリーナの外とボスの体の中には入れない
	private void Move(const Vector3&in delta) {
		Vector3 position = transform.position + delta;

		MadanBoss@ boss = Madan::boss;
		if (boss !is null) {
			const Vector3 center = boss.transform.position;
			const float minDistance = boss.bodyRadius + hitRadius;
			Vector3 away = Madan::Flat(position - center);
			if (away.magnitude < minDistance) {
				if (away.sqrMagnitude < 0.0001f) { away = -facing; }
				const Vector3 pushed = Madan::Flat(center) + away.normalized * minDistance;
				position = Vector3(pushed.x, position.y, pushed.z);
			}
		}

		if (Madan::battle !is null) { position = Madan::battle.ClampToArena(position, hitRadius); }
		transform.position = position;
	}

	private void SetMarkerVisible(bool visible) {
		if (aimMarker is null) { return; }
		if (aimMarker.gameObject.activeSelf != visible) { aimMarker.gameObject.SetActive(visible); }
	}

	// 無敵時間中は点滅させる
	private void UpdateBlink() {
		if (bodyRenderer is null || isDead) { return; }
		const bool blinking = invincibleTimer > 0.0f && hp > 0;
		bodyRenderer.enabled = !blinking || Mathf::Repeat(Time::time * 16.0f, 2.0f) < 1.0f;
	}

	void OnDrawGizmosSelected() {
		Gizmos::color = Color::green;
		Gizmos::DrawWireSphere(transform.position, hitRadius);
	}
}
