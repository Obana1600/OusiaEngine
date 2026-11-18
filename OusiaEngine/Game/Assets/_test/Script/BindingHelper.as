// 確認用: AddComponent<BindingHelper>() で付けるスクリプト
class BindingHelper : MonoBehavior {
	bool awakeCalled = false;

	void Awake() {
		awakeCalled = true;
		Debug::Log("TEST BindingHelper Awake on " + gameObject.name);
	}

	void Start() {
		Debug::Log("TEST BindingHelper Start");
	}
}
