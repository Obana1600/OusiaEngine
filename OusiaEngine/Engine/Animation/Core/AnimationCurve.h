#pragma once

#include "Animation/Core/Keyframe.h"

#include <vector>


namespace ou {

class AnimationCurve {
public: // 静的メンバ関数

	static AnimationCurve Constant(float timeStart, float timeEnd, float value);
	static AnimationCurve Linear(float timeStart, float valueStart, float timeEnd, float valueEnd);
	static AnimationCurve EaseInOut(float timeStart, float valueStart, float timeEnd, float valueEnd);


public: // コンストラクタ

	AnimationCurve() = default;
	explicit AnimationCurve(std::vector<Keyframe> keys);


public: // メンバ関数

	float Evaluate(float time) const;

	int AddKey(float time, float value);
	int AddKey(const Keyframe &key);

	int MoveKey(int index, const Keyframe &key);
	void RemoveKey(int index);

	void SetKeys(std::vector<Keyframe> keys);
	const std::vector<Keyframe> &GetKeys() const;

	int GetLength() const;


public: // オペレータ

	const Keyframe &operator[](int index) const;


private: // メンバ変数

	std::vector<Keyframe> keys_;


private: // メンバ関数

	int InsertKey(const Keyframe &key);
	bool IsValidIndex(int index) const;
};

} // namespace ou
