// 確認用: スクリプトの Material@・AudioClip@ の欄（guid で読み書きする）
class AssetFieldTest : MonoBehavior {
	Material@ mat;
	AudioClip@ clip;
	bool copied = false;

	void Start() {
		Debug::Log("TEST asset copied=" + copied + " mat=" + (mat is null ? "null" : mat.name) + " clip=" + (clip is null ? "null" : clip.name + " len=" + clip.length));
		if (copied) { return; }

		copied = true;
		GameObject@ copy = Instantiate(gameObject);
		AssetFieldTest@ other = copy.GetComponent<AssetFieldTest>();
		Debug::Log("TEST asset copy sameMat=" + (other.mat is mat) + " sameClip=" + (other.clip is clip));
	}
}
