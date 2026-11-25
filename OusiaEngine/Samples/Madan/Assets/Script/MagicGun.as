// 魔弾（仮）: 7 発の魔弾を撃てる銃
// MadanPlayer と同じ GameObject に付ける。弾の並び・残弾・発射を受け持つ
// 各弾の性質の数値もここにまとめ、MagicBullet はここを読む
class MagicGun : MonoBehavior {
	// ---- 弾倉 ----
	// 撃つ弾の並び。使える名前: Normal, Pierce, Homing, Large, Recall, Split, Devil
	// 書いた数がそのまま装弾数になる（企画では 7 発）
	[SerializeField] private string bulletOrder = "Normal, Pierce, Homing, Large, Recall, Split, Devil";
	// true: Q/E で撃つ弾を選べる / false: 並びの順に撃つ
	[SerializeField] private bool allowSelect = false;
	// 撃ってから次を撃てるまで（秒）
	[SerializeField] private float fireCooldown = 0.3f;

	// ---- 弾の見た目と共通の数値 ----
	// 弾の見た目（Sphere を小さくしたものなど）。MagicBullet は自動で付く
	GameObject@ bulletPrefab;
	float bulletSpeed = 20.0f;
	float bulletLifeTime = 2.5f;
	float bulletRadius = 0.2f;
	[SerializeField] private float muzzleHeight = 0.8f;
	[SerializeField] private float muzzleForward = 0.6f;

	// ---- 弾ごとの性質 ----
	float largeRadius = 0.7f;           // 大口径弾の当たり判定の半径
	float homingTurnSpeed = 220.0f;     // 追尾弾の曲がる速さ（度/秒）
	float homingRange = 10.0f;          // 追尾弾が弱点を探す距離
	float splitAngle = 14.0f;           // 散弾の広がる角度（度）
	float recallSpeed = 24.0f;          // 回帰弾が戻ってくる速さ
	// 戻ってきた回帰弾が、もう一度戻る性質を持ち続けるか（true だと実質撃ち放題になる）
	bool recallKeepsProperty = false;
	float devilTurnSpeed = 900.0f;      // 魔王の弾の曲がる速さ（度/秒）
	// 露出中の弱点がないときに撃った魔王の弾が、撃った本人に当たるか
	bool devilHitsShooter = true;

	private array<MadanBulletType> slots;
	private array<bool> spent;
	private int selected = 0;
	private float cooldownTimer = 0.0f;

	void Awake() {
		slots = Madan::ParseBulletOrder(bulletOrder);
		spent.resize(slots.length());
		for (uint i = 0; i < spent.length(); i++) { spent[i] = false; }
		selected = 0;
		if (slots.isEmpty()) { Debug::LogError("MagicGun: bulletOrder に弾がありません"); }
	}

	void Start() {
		MadanBoss@ boss = Madan::boss;
		if (boss !is null && boss.maxHp != capacity) {
			Debug::LogWarning("MagicGun: 装弾数 (" + capacity + ") とボスの体力 (" + boss.maxHp + ") が違います");
		}
	}

	void Update() {
		cooldownTimer -= Time::deltaTime;
	}

	// ---- 状態 ----
	int get_capacity() const property { return int(slots.length()); }
	int get_selectedIndex() const property { return selected; }
	MadanBulletType get_selectedType() const property { return SlotType(selected); }

	int get_remainingBullets() const property {
		int count = 0;
		for (uint i = 0; i < spent.length(); i++) {
			if (!spent[i]) { count++; }
		}
		return count;
	}

	bool IsSpent(int index) const {
		return index < 0 || index >= capacity || spent[uint(index)];
	}

	MadanBulletType SlotType(int index) const {
		if (index < 0 || index >= capacity) { return MadanBulletType::Normal; }
		return slots[uint(index)];
	}

	bool CanFire() const {
		return cooldownTimer <= 0.0f && !IsSpent(selected);
	}

	// ---- 選択 ----
	void SelectNext() {
		if (allowSelect) { selected = FindUnspent(selected + 1, 1); }
	}

	void SelectPrevious() {
		if (allowSelect) { selected = FindUnspent(selected - 1, -1); }
	}

	// ---- 発射 ----
	void Fire(const Vector3&in direction) {
		if (!CanFire()) { return; }
		Vector3 dir = Madan::Flat(direction);
		if (dir.sqrMagnitude < 0.0001f) { dir = Madan::Flat(transform.forward); }
		dir = dir.normalized;

		const int slot = selected;
		const MadanBulletType type = slots[uint(slot)];
		spent[uint(slot)] = true;
		cooldownTimer = fireCooldown;

		const Vector3 origin = transform.position + Vector3::up * muzzleHeight + dir * muzzleForward;
		Debug::Log("[魔弾] " + (slot + 1) + " 発目: " + Madan::BulletLabel(type));

		if (type == MadanBulletType::Split) {
			MadanShotGroup@ group = MadanShotGroup();
			SpawnBullet(type, slot, origin, Madan::RotateY(dir, -splitAngle), group);
			SpawnBullet(type, slot, origin, dir, group);
			SpawnBullet(type, slot, origin, Madan::RotateY(dir, splitAngle), group);
		} else {
			SpawnBullet(type, slot, origin, dir, null);
		}

		// 順番固定なら次の弾、選択式なら同じ方向で次の残っている弾へ
		selected = FindUnspent(slot + 1, 1);
	}

	// 回帰弾が戻ってきたとき（MagicBullet が呼ぶ）
	void Refund(int slot) {
		if (slot < 0 || slot >= capacity) { return; }
		spent[uint(slot)] = false;
		if (!recallKeepsProperty) { slots[uint(slot)] = MadanBulletType::Normal; }
		Debug::Log("[魔弾] 回帰弾が戻った（" + Madan::BulletLabel(slots[uint(slot)]) + " として再装填）");
		// 順番固定のときは、戻った弾を次に撃つ
		if (!allowSelect || IsSpent(selected)) { selected = slot; }
	}

	Vector3 get_shooterPosition() const property {
		return transform.position + Vector3::up * muzzleHeight;
	}

	// start から step 方向へ、まだ撃っていない弾を探す。なければ start をそのまま返す
	private int FindUnspent(int start, int step) const {
		const int count = capacity;
		if (count == 0) { return 0; }
		for (int i = 0; i < count; i++) {
			int index = ((start + step * i) % count + count) % count;
			if (!spent[uint(index)]) { return index; }
		}
		return ((start % count) + count) % count;
	}

	private void SpawnBullet(MadanBulletType type, int slot, const Vector3&in origin, const Vector3&in dir, MadanShotGroup@ group) {
		GameObject@ go;
		if (bulletPrefab !is null) {
			@go = Instantiate(bulletPrefab, origin, Quaternion::LookRotation(dir));
		} else {
			@go = GameObject("MagicBullet");
			go.transform.position = origin;
			Debug::LogWarning("MagicGun: bulletPrefab が未設定のため、弾が見えません");
		}

		MagicBullet@ bullet = go.GetComponent<MagicBullet>();
		if (bullet is null) { @bullet = go.AddComponent<MagicBullet>(); }
		if (bullet is null) {
			Debug::LogError("MagicGun: MagicBullet を付けられませんでした");
			Destroy(go);
			return;
		}
		bullet.Launch(this, type, slot, dir, group);
	}
}
