#pragma once


namespace ou {

enum class EaseType {
	Linear,

	// 正弦曲線
	InSine,
	OutSine,
	InOutSine,

	// 2次関数
	InQuad,
	OutQuad,
	InOutQuad,

	// 3次関数
	InCubic,
	OutCubic,
	InOutCubic,

	// 4次関数
	InQuart,
	OutQuart,
	InOutQuart,

	// 5次関数
	InQuint,
	OutQuint,
	InOutQuint,

	// 指数関数
	InExpo,
	OutExpo,
	InOutExpo,

	// 円弧
	InCirc,
	OutCirc,
	InOutCirc,

	// 戻る
	InBack,
	OutBack,
	InOutBack,

	// 弾性振動
	InElastic,
	OutElastic,
	InOutElastic,

	// バウンド
	InBounce,
	OutBounce,
	InOutBounce,
};


namespace Easing {

float Evaluate(EaseType type, float t);

} // namespace Easing

} // namespace ou
