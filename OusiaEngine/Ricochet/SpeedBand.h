#pragma once

#include "Tuning.h"

#include <cstdint>


namespace ou {

// プレイヤーの状態は速さで 3 段階に分かれる
enum class SpeedBand : uint8_t {
	// ダメージ閾値未満。再発射できる・攻撃なし・アーマーなし・重力あり
	Low,
	// ダメージ閾値〜再発射閾値。再発射できる・攻撃あり・アーマーあり（つなぎの窓）
	Mid,
	// 再発射閾値より上。再発射できない・攻撃あり・アーマーあり
	High,
};

inline SpeedBand ClassifySpeed(float speed) {
	if (speed > Tuning::kRelaunchSpeed) { return SpeedBand::High; }
	if (speed >= Tuning::kDamageSpeed) { return SpeedBand::Mid; }
	return SpeedBand::Low;
}

inline bool CanRelaunch(SpeedBand band) { return band != SpeedBand::High; }
inline bool HasAttack(SpeedBand band) { return band != SpeedBand::Low; }
inline bool HasSuperArmor(SpeedBand band) { return band != SpeedBand::Low; }
inline bool UsesGravity(SpeedBand band) { return band == SpeedBand::Low; }

} // namespace ou
