// 確認用: Button の onClick（Inspector でつないだもの・AddListener でつないだもの）
class ClickTest : MonoBehavior {
	Button@ button;
	int runtimeCount = 0;

	void OnHello(string word) { Debug::Log("TEST persistent string=" + word); }
	void OnCount(int n) { Debug::Log("TEST persistent int=" + n); }
	void OnRuntime() { runtimeCount++; Debug::Log("TEST runtime count=" + runtimeCount); }

	void Start() {
		Debug::Log("TEST persistentCount=" + button.onClick.GetPersistentEventCount() + " name0=" + button.onClick.GetPersistentMethodName(0) + " interactable=" + button.interactable);
		button.onClick.AddListener(UnityAction(this.OnRuntime));
		Debug::Log("TEST invoke 1");
		button.onClick.Invoke();
		button.onClick.RemoveListener(UnityAction(this.OnRuntime));
		Debug::Log("TEST invoke 2 (removed)");
		button.onClick.Invoke();
		button.onClick.AddListener(UnityAction(this.OnRuntime));
		button.onClick.RemoveAllListeners();
		Debug::Log("TEST invoke 3 (RemoveAll, persistent remain)");
		button.onClick.Invoke();

		UnityEvent@ kept = button.onClick;
		Object::Destroy(button.gameObject);
	}

	int frame = 0;
	void Update() {
		if (++frame == 2) {
			Debug::Log("TEST after destroy (expect MissingReference)");
			UnityEvent@ e = button.onClick;
		}
		if (frame == 3) { Application::Quit(); }
	}
}
