#include "ComboCounter.h"
#include "Core/Time.h"
#include "Math/Mathf.h"
#include "Player.h"
#include "Tuning.h"

#include <algorithm>

using namespace ou;



void ou::ComboCounter::Awake() {
	sInstance_ = this;
}


void ou::ComboCounter::Update() {
	if (combo_ == 0) { return; }
	timer_ -= Time::GetDeltaTime();
	if (timer_ <= 0.0f) { combo_ = 0; }
}


void ou::ComboCounter::OnDestroy() {
	if (sInstance_ == this) { sInstance_ = nullptr; }
}


void ou::ComboCounter::Add() {
	combo_++;
	maxCombo_ = std::max(maxCombo_, combo_);
	timer_ = Tuning::kComboResetTime;

	// 10 コンボごとにストックを 1 回復する
	if (Tuning::kComboRecoversStock && combo_ % Tuning::kComboPerStock == 0) {
		if (auto *player = Player::GetInstance()) { player->AddStock(1); }
	}
}


float ou::ComboCounter::GetRemainingRatio() const {
	if (combo_ == 0) { return 0.0f; }
	return Mathf::Clamp01(timer_ / Tuning::kComboResetTime);
}
