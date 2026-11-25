// 魔弾（仮）: ボスの弱点
// ボスの子の GameObject に付ける。位置はボスの体に固定され、ボスと一緒に回る
// MadanBoss が段階に合わせて開け閉めする
class WeakPoint : MonoBehavior {
	// 何段階目で露出するか（0 始まり、ボスに 1 発当てるごとに 1 進む）
	// "0" や "2, 5" のように書く。同じ段階の弱点が複数あると同時に露出し、どれか 1 つに当てれば次へ進む
	[SerializeField] private string stages = "0";
	// 当たり判定の半径
	float radius = 0.5f;

	// 見た目（MeshRenderer があるときだけ）
	[SerializeField] private Color closedColor = Color(0.35f, 0.35f, 0.4f);
	[SerializeField] private Color warningColor = Color(1.0f, 0.75f, 0.2f);
	[SerializeField] private Color openColor = Color(1.0f, 0.15f, 0.15f);
	[SerializeField] private Color brokenColor = Color(0.08f, 0.08f, 0.08f);
	// 露出中にふくらませる倍率
	[SerializeField] private float openScale = 1.35f;

	private array<int> stageList;
	private bool open = false;
	private bool warning = false;
	private bool broken = false;
	private Vector3 baseScale = Vector3::one;
	private MeshRenderer@ meshRenderer;

	void Awake() {
		stageList = Madan::ParseIntList(stages);
		baseScale = transform.localScale;
		@meshRenderer = gameObject.GetComponent<MeshRenderer>();
		ApplyLook();
	}

	void Update() {
		Vector3 scale = baseScale;
		if (open) {
			const float pulse = 1.0f + 0.08f * Mathf::Sin(Time::time * 14.0f);
			scale = baseScale * (openScale * pulse);
		}
		transform.localScale = scale;

		if (warning && meshRenderer !is null) {
			// 予兆は点滅
			const bool on = Mathf::Repeat(Time::time * 10.0f, 2.0f) < 1.0f;
			meshRenderer.material.color = on ? warningColor : closedColor;
		}
	}

	bool get_isOpen() const property { return open; }
	Vector3 get_center() const property { return transform.position; }

	bool HasStage(int stage) const {
		return stageList.find(stage) >= 0;
	}

	// 最後の露出の段階。これより後に開くことはない
	int get_lastStage() const property {
		int last = -1;
		for (uint i = 0; i < stageList.length(); i++) { last = Mathf::Max(last, stageList[i]); }
		return last;
	}

	void SetWarning(bool value) {
		warning = value && !broken;
		if (!warning) { ApplyLook(); }
	}

	void SetOpen(bool value) {
		open = value && !broken;
		warning = false;
		ApplyLook();
	}

	// 当てられたあと、もう開かない弱点はつぶれた見た目にする
	void MarkBroken() {
		broken = true;
		open = false;
		warning = false;
		ApplyLook();
	}

	private void ApplyLook() {
		if (meshRenderer is null) { return; }
		if (broken) { meshRenderer.material.color = brokenColor; }
		else if (open) { meshRenderer.material.color = openColor; }
		else { meshRenderer.material.color = closedColor; }
	}

	void OnDrawGizmos() {
		Gizmos::color = open ? Color::red : Color::yellow;
		Gizmos::DrawWireSphere(transform.position, radius);
	}
}
