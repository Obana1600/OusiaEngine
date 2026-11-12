#pragma once


namespace ou {

enum class WeightedMode {
	None,
	In,
	Out,
	Both,
};

// アニメーションカーブでの単一のキーフレーム
struct Keyframe {
public: // メンバ変数

	float time;				   // キーフレームの時間
	float value;			   // キーフレームでのカーブの値
	float inTangent;		   // 入力接線の傾き
	float outTangent;		   // 出力接線の傾き
	float inWeight;			   // 入力接線の重み
	float outWeight;		   // 出力接線の重み
	WeightedMode weightedMode; // 接線の重みをどう扱うか


public: // コンストラクタ

	Keyframe();
	Keyframe(float time, float value);
	Keyframe(float time, float value, float inTangent, float outTangent);
	Keyframe(float time, float value, float inTangent, float outTangent, float inWeight, float outWeight);
};

} // namespace ou
