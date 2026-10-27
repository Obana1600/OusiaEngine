#pragma once

#include "Scene/Component.h"

#include <cstdint>


namespace ou {

// 勝敗と形態の移行の進行。終わったら R でやり直す
class GameFlow final: public Component {
public: // メンバ関数

	void Start() override;
	void Update() override;


private: // インナークラス

	enum class State : uint8_t {
		Playing,
		Cleared,
		GameOver,
	};


private: // メンバ変数

	State state_ = State::Playing;
	int lastPhase_ = 1;
};

} // namespace ou
