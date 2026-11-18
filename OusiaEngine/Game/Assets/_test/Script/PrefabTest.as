// 確認用: スクリプトの欄に入れたプレハブ（.prefab）から Instantiate する
class PrefabTest : MonoBehavior {
	GameObject@ bulletPrefab;
	int frame = 0;

	void Start() {
		Debug::Log("TEST prefab name=" + bulletPrefab.name + " in scene=" + (GameObject::Find("Bullet") !is null)
				   + " children=" + bulletPrefab.transform.childCount + " active=" + bulletPrefab.activeInHierarchy);
		PrefabProbe@ probe = bulletPrefab.GetComponent<PrefabProbe>();
		Debug::Log("TEST prefab probe damage=" + probe.damage + " self=" + (probe.self is bulletPrefab));

		for (int i = 0; i < 3; i++) {
			GameObject@ bullet = Instantiate(bulletPrefab, Vector3(i, 0, 0), Quaternion::identity);
			PrefabProbe@ p = bullet.GetComponent<PrefabProbe>();
			Debug::Log("TEST bullet " + i + " name=" + bullet.name + " pos=" + bullet.transform.position.ToString()
					   + " scale=" + bullet.transform.localScale.ToString() + " self=" + (p.self is bullet)
					   + " child=" + (p.child is bullet.transform.GetChild(0)) + " damage=" + p.damage
					   + " rigidbody=" + (bullet.GetComponent<Rigidbody>() !is null));
		}

		// プレハブを指す欄を持つものを複製する（JsonWriter が guid で書き、JsonReader がアセットから引く）
		GameObject@ holder = GameObject("Holder");
		@holder.AddComponent<PrefabProbe>().other = bulletPrefab;
		GameObject@ copied = Instantiate(holder);
		Debug::Log("TEST copied other is prefab=" + (copied.GetComponent<PrefabProbe>().other is bulletPrefab));
	}

	void Update() {
		if (++frame == 3) { Application::Quit(); }
	}
}
