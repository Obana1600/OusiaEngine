// 魔弾（仮）: 戦闘全体の進行役
// 勝ち負けの判定、HUD の表示、リスタートを受け持つ。シーンに 1 つだけ置く
class MadanBattle : MonoBehavior {
	// ---- 参照（空なら名前で探す） ----
	GameObject@ playerObject; // 空なら "Player"
	GameObject@ bossObject;   // 空なら "Boss"
	Text@ hudText;            // 弾とボスの体力の表示（任意）
	Text@ messageText;        // 勝敗の表示（任意）

	// ---- ルール ----
	// 残りの弾ではボスを倒しきれなくなった時点で敗北にするか
	// false なら、撃ち尽くして飛んでいる弾がなくなった時点で敗北
	[SerializeField] private bool loseWhenUnwinnable = false;
	// アリーナの半径。プレイヤーはこの外に出られず、弾はこの外で消える
	float arenaRadius = 12.0f;
	// 戦闘開始までの待ち時間（秒）
	[SerializeField] private float readyTime = 1.0f;
	// 勝敗が決まってからリスタートを受け付けるまで（秒）
	[SerializeField] private float restartDelay = 1.0f;

	MadanBattleState state = MadanBattleState::Ready;
	private float stateTimer = 0.0f;
	private int bulletsInFlight = 0;
	private string resultMessage;
	private MagicGun@ gun;

	void Awake() {
		@Madan::battle = this;
	}

	void OnDestroy() {
		if (Madan::battle is this) { @Madan::battle = null; }
	}

	void Start() {
		if (playerObject is null) { @playerObject = GameObject::Find("Player"); }
		if (bossObject is null) { @bossObject = GameObject::Find("Boss"); }
		if (playerObject is null) { Debug::LogError("MadanBattle: プレイヤーが見つかりません（playerObject を設定してください）"); }
		if (bossObject is null) { Debug::LogError("MadanBattle: ボスが見つかりません（bossObject を設定してください）"); }
		if (playerObject !is null) { @gun = playerObject.GetComponent<MagicGun>(); }

		SetState(MadanBattleState::Ready);
	}

	void Update() {
		stateTimer += Time::deltaTime;

		switch (state) {
		case MadanBattleState::Ready:
			if (stateTimer >= readyTime) { SetState(MadanBattleState::Fighting); }
			break;
		case MadanBattleState::Fighting:
			CheckResult();
			break;
		case MadanBattleState::Won:
		case MadanBattleState::Lost:
			if (stateTimer >= restartDelay && (Input::GetKeyDown(KeyCode::R) || Input::GetKeyDown(KeyCode::Return))) {
				Restart();
			}
			break;
		}

		UpdateHud();
	}

	// ---- 弾の出入り（MagicBullet が呼ぶ） ----
	void RegisterBullet() { bulletsInFlight++; }
	void UnregisterBullet() { bulletsInFlight = Mathf::Max(0, bulletsInFlight - 1); }
	int get_flyingBullets() const property { return bulletsInFlight; }

	// プレイヤーや弾がアリーナの中に収まる位置
	Vector3 ClampToArena(const Vector3&in position, float margin) const {
		Vector3 flat = Madan::Flat(position);
		const float limit = Mathf::Max(0.0f, arenaRadius - margin);
		if (flat.magnitude > limit) { flat = flat.normalized * limit; }
		return Vector3(flat.x, position.y, flat.z);
	}

	bool IsOutsideArena(const Vector3&in position, float margin) const {
		return Madan::Flat(position).magnitude > arenaRadius + margin;
	}

	void Lose(const string&in reason) {
		if (state != MadanBattleState::Fighting) { return; }
		resultMessage = "敗北 … " + reason + "\nR でリトライ";
		SetState(MadanBattleState::Lost);
	}

	private void Win() {
		resultMessage = "勝利！\nR でもう一度";
		SetState(MadanBattleState::Won);
	}

	private void CheckResult() {
		MadanBoss@ boss = Madan::boss;
		MadanPlayer@ player = Madan::player;

		if (boss !is null && boss.hp <= 0) { Win(); return; }
		if (player !is null && player.isDead) { Lose("攻撃を受けた"); return; }

		// 飛んでいる弾の結果が出るまでは判定しない（回帰弾が戻ってくることもある）
		if (gun is null || boss is null || bulletsInFlight > 0) { return; }
		const int remaining = gun.remainingBullets;
		if (remaining <= 0) {
			Lose("魔弾が尽きた");
		} else if (loseWhenUnwinnable && remaining < boss.hp) {
			Lose("残りの魔弾では倒しきれない");
		}
	}

	private void SetState(MadanBattleState next) {
		state = next;
		stateTimer = 0.0f;
		if (next == MadanBattleState::Won) { Debug::Log("[魔弾] 勝利"); }
		if (next == MadanBattleState::Lost) { Debug::Log("[魔弾] " + resultMessage); }

		if (messageText !is null) {
			switch (next) {
			case MadanBattleState::Ready: messageText.text = "READY"; break;
			case MadanBattleState::Fighting: messageText.text = ""; break;
			default: messageText.text = resultMessage; break;
			}
		}
	}

	private void Restart() {
		SceneManager::LoadScene(SceneManager::GetActiveScene().name);
	}

	private void UpdateHud() {
		if (hudText is null) { return; }

		string text = "";
		if (gun !is null) {
			text += "魔弾 ";
			for (int i = 0; i < gun.capacity; i++) {
				text += gun.IsSpent(i) ? "-" : (i == gun.selectedIndex ? "[*]" : "*");
			}
			text += "  " + gun.remainingBullets + "/" + gun.capacity;
			if (gun.remainingBullets > 0) {
				const MadanBulletType next = gun.selectedType;
				text += "\n次の弾: " + Madan::BulletLabel(next) + "（" + Madan::BulletDescription(next) + "）";
			}
		}
		MadanBoss@ boss = Madan::boss;
		if (boss !is null) {
			text += "\nBOSS ";
			for (int i = 0; i < boss.maxHp; i++) { text += i < boss.hp ? "#" : "_"; }
		}
		MadanPlayer@ player = Madan::player;
		if (player !is null && player.maxHp > 1) {
			text += "\nHP " + player.hp + "/" + player.maxHp;
		}
		hudText.text = text;
	}
}
