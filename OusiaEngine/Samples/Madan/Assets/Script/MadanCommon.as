// 魔弾（仮）: 全スクリプトで使う定義
// MonoBehavior ではないので、GameObject には付けない

// 魔弾の性質。並び順は MagicGun.bulletOrder の文字列で決める
enum MadanBulletType {
	Normal, // 通常弾: まっすぐ飛ぶ
	Pierce, // 貫通弾: ボスの体を素通りし、裏側の弱点にも届く
	Homing, // 追尾弾: 露出中の弱点へ向かって曲がる
	Large,  // 大口径弾: 当たり判定が大きい
	Recall, // 回帰弾: 外れると手元に戻ってくる（1 回だけ）
	Split,  // 散弾: 3 方向に分かれる。どれか 1 発が当たれば 1 ダメージ
	Devil   // 魔王の弾: 露出中の弱点に必ず当たる。露出中の弱点がなければ撃った本人を狙う
}

enum MadanBattleState {
	Ready,    // 開始前の待ち
	Fighting, // 戦闘中
	Won,      // 勝利
	Lost      // 敗北
}

namespace Madan {
	// 戦闘中のオブジェクト。それぞれの Awake で入り、OnDestroy で外れる
	MadanBattle@ battle;
	MadanBoss@ boss;
	MadanPlayer@ player;

	bool IsFighting() {
		return battle is null || battle.state == MadanBattleState::Fighting;
	}

	// 高さを無視した XZ 平面のベクトル
	Vector3 Flat(const Vector3&in v) {
		return Vector3(v.x, 0.0f, v.z);
	}

	float FlatDistance(const Vector3&in a, const Vector3&in b) {
		return Flat(a - b).magnitude;
	}

	// Y 軸まわりに degrees 度まわす
	Vector3 RotateY(const Vector3&in v, float degrees) {
		return Quaternion::Euler(0.0f, degrees, 0.0f) * v;
	}

	// 線分 from→to と点 p のいちばん近い距離（XZ 平面）。速い弾のすり抜け防止
	float SegmentPointDistance(const Vector3&in from, const Vector3&in to, const Vector3&in p) {
		const Vector3 a = Flat(from);
		const Vector3 b = Flat(to);
		const Vector3 c = Flat(p);
		const Vector3 ab = b - a;
		const float lengthSq = ab.sqrMagnitude;
		if (lengthSq < 0.000001f) { return (c - a).magnitude; }
		const float t = Mathf::Clamp01(Vector3::Dot(c - a, ab) / lengthSq);
		return (c - (a + ab * t)).magnitude;
	}

	string Trim(const string&in text) {
		const int first = text.findFirstNotOf(" \t\r\n");
		if (first < 0) { return ""; }
		const int last = text.findLastNotOf(" \t\r\n");
		return text.substr(uint(first), last - first + 1);
	}

	// separator で区切り、前後の空白を取り除く。空の要素は捨てる
	array<string>@ Split(const string&in text, const string&in separator) {
		array<string> result;
		uint start = 0;
		while (start <= text.length()) {
			int found = text.findFirst(separator, start);
			const uint end = found < 0 ? text.length() : uint(found);
			const string part = Trim(text.substr(start, int(end - start)));
			if (!part.isEmpty()) { result.insertLast(part); }
			if (found < 0) { break; }
			start = end + separator.length();
		}
		return result;
	}

	bool TryParseBulletType(const string&in text, MadanBulletType&out type) {
		const string name = Trim(text);
		if (name == "Normal") { type = MadanBulletType::Normal; return true; }
		if (name == "Pierce") { type = MadanBulletType::Pierce; return true; }
		if (name == "Homing") { type = MadanBulletType::Homing; return true; }
		if (name == "Large") { type = MadanBulletType::Large; return true; }
		if (name == "Recall") { type = MadanBulletType::Recall; return true; }
		if (name == "Split") { type = MadanBulletType::Split; return true; }
		if (name == "Devil") { type = MadanBulletType::Devil; return true; }
		type = MadanBulletType::Normal;
		return false;
	}

	string BulletLabel(MadanBulletType type) {
		switch (type) {
		case MadanBulletType::Normal: return "通常弾";
		case MadanBulletType::Pierce: return "貫通弾";
		case MadanBulletType::Homing: return "追尾弾";
		case MadanBulletType::Large: return "大口径弾";
		case MadanBulletType::Recall: return "回帰弾";
		case MadanBulletType::Split: return "散弾";
		case MadanBulletType::Devil: return "魔王の弾";
		}
		return "?";
	}

	string BulletDescription(MadanBulletType type) {
		switch (type) {
		case MadanBulletType::Normal: return "まっすぐ飛ぶ";
		case MadanBulletType::Pierce: return "ボスの体を貫通する";
		case MadanBulletType::Homing: return "露出中の弱点へ曲がる";
		case MadanBulletType::Large: return "当たり判定が大きい";
		case MadanBulletType::Recall: return "外れると一度だけ手元に戻る";
		case MadanBulletType::Split: return "3 方向に分かれる";
		case MadanBulletType::Devil: return "露出中の弱点に必ず当たる。なければ自分に当たる";
		}
		return "";
	}

	// "Normal, Pierce, ..." を弾の並びにする。読めない名前は Normal にして警告を出す
	array<MadanBulletType>@ ParseBulletOrder(const string&in csv) {
		array<MadanBulletType> result;
		array<string>@ names = Split(csv, ",");
		for (uint i = 0; i < names.length(); i++) {
			MadanBulletType type;
			if (!TryParseBulletType(names[i], type)) {
				Debug::LogWarning("魔弾の名前を読めません: " + names[i] + "（Normal として扱います）");
			}
			result.insertLast(type);
		}
		return result;
	}

	// "0, 3" のような整数の並び
	array<int>@ ParseIntList(const string&in csv) {
		array<int> result;
		array<string>@ parts = Split(csv, ",");
		for (uint i = 0; i < parts.length(); i++) {
			result.insertLast(int(parseInt(parts[i])));
		}
		return result;
	}
}
