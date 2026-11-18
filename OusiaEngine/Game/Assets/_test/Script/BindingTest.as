// 確認用: バインドだけで足した API（GetComponent<T>・AddComponent<T>・Find・Random・SceneManager・Audio・Renderer・UI）
class BindingTest : MonoBehavior {
	int frame = 0;

	void Start() {
		// GetComponent<T>（C++ の型・スクリプトのクラス・ないもの）
		Transform@ t = gameObject.GetComponent<Transform>();
		Debug::Log("TEST GetComponent<Transform> same=" + (t is transform));
		BindingTest@ self = gameObject.GetComponent<BindingTest>();
		Debug::Log("TEST GetComponent<BindingTest> same=" + (self is this));
		Debug::Log("TEST GetComponent<Rigidbody> null=" + (gameObject.GetComponent<Rigidbody>() is null));

		// AddComponent<T>（C++ の型とスクリプトのクラス）
		GameObject@ go = GameObject("Spawned");
		Rigidbody@ rb = go.AddComponent<Rigidbody>();
		rb.useGravity = false;
		BoxCollider@ box = go.AddComponent<BoxCollider>();
		box.size = Vector3(2, 3, 4);
		Debug::Log("TEST AddComponent Rigidbody=" + (rb !is null) + " BoxCollider size=" + box.size.ToString()
				   + " found=" + (go.GetComponent<Collider>() is box));
		BindingHelper@ helper = go.AddComponent<BindingHelper>();
		Debug::Log("TEST AddComponent<BindingHelper> awake=" + (helper !is null ? helper.awakeCalled : false));

		// GameObject::Find と cast
		GameObject@ found = GameObject::Find("Spawned");
		Debug::Log("TEST Find=" + (found is go) + " missing=" + (GameObject::Find("NoSuchObject") is null));
		Object@ obj = rb;
		Debug::Log("TEST cast<Rigidbody>=" + (cast<Rigidbody>(obj) is rb) + " cast<Collider>=" + (cast<Collider>(obj) is null));

		// Random
		Random::InitState(42);
		int i = Random::Range(0, 3);
		float f = Random::Range(1.0f, 2.0f);
		Debug::Log("TEST Random int=" + i + " float=" + f + " value=" + Random::value + " sphere=" + Random::insideUnitSphere.magnitude);

		// SceneManager
		Scene scene = SceneManager::GetActiveScene();
		Debug::Log("TEST Scene name=" + scene.name + " buildIndex=" + scene.buildIndex + " valid=" + scene.IsValid()
				   + " count=" + SceneManager::sceneCountInBuildSettings);

		// Audio・Renderer・UI は、シーンにあれば触る
		AudioSource@ audio = gameObject.GetComponent<AudioSource>();
		if (audio !is null) {
			Debug::Log("TEST AudioSource clip=" + (audio.clip !is null ? audio.clip.name : "null") + " volume=" + audio.volume);
		}
		Renderer@ renderer = gameObject.GetComponent<Renderer>();
		if (renderer !is null) {
			renderer.material.color = Color::red;
			Debug::Log("TEST Renderer material=" + renderer.material.name + " color=" + renderer.material.color.ToString());
		}
		Text@ text = gameObject.GetComponent<Text>();
		if (text !is null) {
			text.text = "スコア: 0";
			Debug::Log("TEST Text=" + text.text);
		}

		DontDestroyOnLoad(go);
		Debug::Log("TEST Application isPlaying=" + Application::isPlaying);
	}

	void Update() {
		frame++;
		if (frame == 3) {
			// 例外になる（Rigidbody は AddComponent できるが Renderer は抽象）
			gameObject.AddComponent<Renderer>();
		}
	}
}

