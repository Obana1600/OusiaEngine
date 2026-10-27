#pragma once

#include "Math/Color.h"
#include "SpeedBand.h"


namespace ou::GameColors {

// プレイヤーの速度帯の色（箱・オーラ・ライト・パーティクルでそろえる）
inline const Color kPlayerLow = {0.55f, 0.6f, 0.75f, 1.0f};
inline const Color kPlayerMid = {0.2f, 1.0f, 0.9f, 1.0f};
inline const Color kPlayerHigh = {1.0f, 0.75f, 0.2f, 1.0f};
inline const Color kPlayerHit = {1.0f, 0.15f, 0.15f, 1.0f};

// 自動突進・弱点など「ご褒美」の金色
inline const Color kGold = {1.0f, 0.85f, 0.3f, 1.0f};

inline const Color kSmallBullet = {1.0f, 0.35f, 0.5f, 1.0f};
inline const Color kHomingBullet = {0.75f, 0.35f, 1.0f, 1.0f};
inline const Color kGiantBullet = {1.0f, 0.5f, 0.1f, 1.0f};
inline const Color kBossDanger = {1.0f, 0.25f, 0.2f, 1.0f};

inline Color ForBand(SpeedBand band) {
	switch (band) {
		case SpeedBand::High: return kPlayerHigh;
		case SpeedBand::Mid: return kPlayerMid;
		case SpeedBand::Low: return kPlayerLow;
	}
	return kPlayerLow;
}

} // namespace ou::GameColors
