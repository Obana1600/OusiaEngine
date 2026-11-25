// 魔弾（仮）: 見下ろし（クォータービュー）のカメラ
// カメラの GameObject に付ける。プレイヤーとボスの間を見るように追いかける
class QuarterViewCamera : MonoBehavior {
	Transform@ target;     // 追いかける相手（空なら "Player"）
	Transform@ subTarget;  // 一緒に画面に入れたい相手（空なら "Boss"）

	// 見下ろす角度（度）。90 で真上から、45 前後で斜め見下ろし
	[SerializeField] private float pitch = 55.0f;
	// 水平方向の向き（度）。WASD の向きはカメラに合わせて自動で変わる
	[SerializeField] private float yaw = 0.0f;
	[SerializeField] private float distance = 18.0f;
	// 0 でプレイヤーだけを見る、0.5 でプレイヤーとボスのちょうど中間を見る
	[SerializeField] private float subTargetWeight = 0.3f;
	// 追いかける速さ。大きいほどぴったり付いてくる
	[SerializeField] private float followSharpness = 6.0f;

	private bool snapped = false;

	void Start() {
		if (target is null) {
			GameObject@ go = GameObject::Find("Player");
			if (go !is null) { @target = go.transform; }
		}
		if (subTarget is null) {
			GameObject@ go = GameObject::Find("Boss");
			if (go !is null) { @subTarget = go.transform; }
		}
	}

	void LateUpdate() {
		if (target is null) { return; }

		Vector3 focus = target.position;
		if (subTarget !is null) { focus = Vector3::Lerp(focus, subTarget.position, subTargetWeight); }

		const Quaternion rotation = Quaternion::Euler(pitch, yaw, 0.0f);
		const Vector3 desired = focus - rotation * Vector3::forward * distance;

		if (!snapped) {
			transform.position = desired;
			snapped = true;
		} else {
			const float t = 1.0f - Mathf::Exp(-followSharpness * Time::deltaTime);
			transform.position = Vector3::Lerp(transform.position, desired, t);
		}
		transform.rotation = rotation;
	}
}
