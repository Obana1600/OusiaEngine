// 確認用（4-2）: TryGetComponent・Raycast・当たり判定のメッセージ
class Bouncer : MonoBehavior {
	int hitCount = 0;

	void Start() {
		Rigidbody@ rb;
		if (gameObject.TryGetComponent(@rb)) {
			Debug::Log("TEST TryGetComponent Rigidbody mass=" + rb.mass + " useGravity=" + rb.useGravity);
		}

		Mover@ mover;
		Debug::Log("TEST TryGetComponent Mover=" + gameObject.TryGetComponent(@mover));

		Bouncer@ self;
		if (gameObject.TryGetComponent(@self)) {
			Debug::Log("TEST TryGetComponent Bouncer same=" + (self is this));
		}

		

		RaycastHit hit;
		if (Physics::Raycast(transform.position, Vector3::down, hit)) {
			Debug::Log("TEST Raycast hit=" + hit.collider.gameObject.name + " distance=" + hit.distance + " normal=" + hit.normal.ToString());
		}
	}

	void OnCollisionEnter(Collision collision) {
		hitCount++;
		Debug::Log("TEST OnCollisionEnter " + collision.gameObject.name + " contacts=" + collision.contactCount
				   + " normal=" + collision.GetContact(0).normal.ToString());

		// 1 回目に当たったら、地面をトリガーにして落ちていく（OnTriggerEnter・Exit を確かめる）
		if (hitCount == 1) {
			collision.collider.isTrigger = true;
		}
	}

	void OnCollisionExit(Collision collision) {
		Debug::Log("TEST OnCollisionExit " + collision.gameObject.name);
	}

	void OnTriggerEnter(Collider@ other) {
		Debug::Log("TEST OnTriggerEnter " + other.gameObject.name);
	}

	void OnTriggerExit(Collider@ other) {
		Debug::Log("TEST OnTriggerExit " + other.gameObject.name);
	}
}
