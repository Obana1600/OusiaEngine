abstract class MonoBehavior {
	private Behavior@ native_;

	// テスト

	GameObject@ gameObject { get const { return native_.gameObject; } }
	Transform@ transform { get const { return native_.transform; } }
	string name { get const { return native_.name; } set { native_.name = value; } }
	bool enabled { get const { return native_.enabled; } set { native_.enabled = value; } }
	bool isActiveAndEnabled { get const { return native_.isActiveAndEnabled; } }

	void Destroy(Object@ obj) { Object::Destroy(obj); }
}
