// 確認用: Button の onClick を目で確かめる
// 押されると label の文字が変わる（エディタにはログの窓がないので、Game ビューで見えるようにしている）
class ButtonDemo : MonoBehavior {
	Text@ label;     // 結果を出す Text（Hierarchy から落とす）
	Button@ button;  // AddListener でもつなぐ Button（空なら、スクリプトからはつながない）

	int inspectorClicks = 0;
	int scriptClicks = 0;

	void Start() {
		if (button !is null) { button.onClick.AddListener(UnityAction(this.OnClickFromScript)); }
		Show("まだ押されていません");
	}

	// Inspector の onClick で選ぶ（引数なし）
	void OnClickHello() {
		inspectorClicks++;
		Show("Inspector: " + inspectorClicks + " 回 / Script: " + scriptClicks + " 回");
	}

	// Inspector の onClick で選び、「渡す値」に文字を書く
	void OnClickWithText(string message) {
		Show("渡された文字: " + message);
	}

	// AddListener でつないだもの
	void OnClickFromScript() {
		scriptClicks++;
		Show("Inspector: " + inspectorClicks + " 回 / Script: " + scriptClicks + " 回");
	}

	void Show(string text) {
		Debug::Log(text);
		if (label !is null) { label.text = text; }
	}
}
