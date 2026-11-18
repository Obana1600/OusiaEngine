// 確認用: Object::Instantiate（GameObject とその子の複製・参照のつなぎ直し・Awake の順番）
class InstantiateTest : MonoBehavior {
	void Start() {
		GameObject@ original = GameObject("Original");
		original.transform.position = Vector3(1, 2, 3);
		GameObject@ child = GameObject("Child");
		@child.transform.parent = original.transform;
		child.transform.localPosition = Vector3(0, 1, 0);
		child.AddComponent<Rigidbody>().mass = 7;

		CloneProbe@ probe = original.AddComponent<CloneProbe>();
		@probe.inner = child.transform;
		@probe.outer = gameObject;
		probe.value = 42;

		Debug::Log("TEST before Instantiate");
		GameObject@ clone = Instantiate(original);
		Debug::Log("TEST after Instantiate name=" + clone.name + " pos=" + clone.transform.position.ToString()
				   + " childCount=" + clone.transform.childCount);

		CloneProbe@ cloned = clone.GetComponent<CloneProbe>();
		Transform@ clonedChild = clone.transform.GetChild(0);
		Debug::Log("TEST inner is clone child=" + (cloned.inner is clonedChild) + " not original=" + (cloned.inner !is child.transform)
				   + " outer same=" + (cloned.outer is gameObject) + " value=" + cloned.value
				   + " child mass=" + clonedChild.GetComponent<Rigidbody>().mass);

		GameObject@ placed = Instantiate(original, Vector3(10, 0, 0), Quaternion::Euler(0, 90, 0));
		Debug::Log("TEST at position pos=" + placed.transform.position.ToString() + " rot=" + placed.transform.eulerAngles.ToString());

		GameObject@ parented = Instantiate(original, transform);
		Debug::Log("TEST with parent parent=" + parented.transform.parent.name + " local=" + parented.transform.localPosition.ToString());

		// 親を付ける版でも、Awake のときにワールドの位置が決まっている（CloneProbe の Awake のログで見る）
		GameObject@ placedChild = Instantiate(original, Vector3(0, 5, 0), Quaternion::Euler(0, 45, 0), transform);
		Debug::Log("TEST at position with parent pos=" + placedChild.transform.position.ToString() + " rot=" + placedChild.transform.eulerAngles.ToString()
				   + " scale=" + placedChild.transform.localScale.ToString());
		GameObject@ worldSpace = Instantiate(original, transform, true);
		Debug::Log("TEST worldSpace pos=" + worldSpace.transform.position.ToString());

		GameObject@ fromChild = Instantiate(child);
		Debug::Log("TEST from child parent null=" + (fromChild.transform.parent is null) + " pos=" + fromChild.transform.position.ToString());

		Debug::Log("TEST null=" + (Instantiate(null) is null));
	}
}
