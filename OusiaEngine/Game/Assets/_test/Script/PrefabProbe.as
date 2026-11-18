// 確認用: プレハブに付けるスクリプト（中への参照・プレハブへの参照・値）
class PrefabProbe : MonoBehavior {
	int damage = 1;
	GameObject@ self;
	Transform@ child;
	GameObject@ other;

	void Awake() {
		Debug::Log("TEST PrefabProbe Awake " + gameObject.name);
	}
}
