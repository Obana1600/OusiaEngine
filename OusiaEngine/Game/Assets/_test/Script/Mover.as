// 確認用（3-2）: 変数が Inspector に出て、保存して Play しても残るか
void Foo(){
	Debug::Log("AAAAA");
}

class Mover : MonoBehavior {
	float speed = 3.0f;
	int count = 2;
	bool flag = true;
	string label = "hello";
	Vector3 offset = Vector3(1, 2, 3);
	private Color tint = Color::red;
	private Transform@ target;

	[SerializeField] private float secret = 7.0f;
	private float hidden = 9.0f;

	void Awake() {
		Foo();
	}

	void Start() {
		Debug::Log("TEST fields speed=" + speed + " count=" + count + " flag=" + flag + " label=" + label
				   + " offset=" + offset.ToString() + " tint=" + tint.ToString() + " secret=" + secret + " hidden=" + hidden);
		Debug::Log("TEST target=" + (target is null ? "null" : target.gameObject.name));
	}
}
