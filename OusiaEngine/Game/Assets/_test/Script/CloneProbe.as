// 確認用: Instantiate で複製される側（中への参照・外への参照・値）
class CloneProbe : MonoBehavior {
	Transform@ inner;
	GameObject@ outer;
	int value = 0;

	void Awake() {
		Debug::Log("TEST CloneProbe Awake " + gameObject.name + " pos=" + transform.position.ToString()
				   + " parent=" + (transform.parent is null ? "null" : transform.parent.name));
	}

	void Start() {
		Debug::Log("TEST CloneProbe Start " + gameObject.name);
	}
}
