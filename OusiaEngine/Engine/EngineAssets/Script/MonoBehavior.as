abstract class MonoBehavior {
	private Behavior@ native_;

	// テスト

	GameObject@ gameObject { get const { return native_.gameObject; } }
	Transform@ transform { get const { return native_.transform; } }
	string name { get const { return native_.name; } set { native_.name = value; } }
	bool enabled { get const { return native_.enabled; } set { native_.enabled = value; } }
	bool isActiveAndEnabled { get const { return native_.isActiveAndEnabled; } }

	void Destroy(Object@ obj) { Object::Destroy(obj); }
	void DontDestroyOnLoad(Object@ target) { Object::DontDestroyOnLoad(target); }
	GameObject@ Instantiate(GameObject@ original) { return Object::Instantiate(original); }
	GameObject@ Instantiate(GameObject@ original, Transform@ parent, bool instantiateInWorldSpace = false) { return Object::Instantiate(original, parent, instantiateInWorldSpace); }
	GameObject@ Instantiate(GameObject@ original, const Vector3 &in position, const Quaternion &in rotation, Transform@ parent = null) { return Object::Instantiate(original, position, rotation, parent); }
}
