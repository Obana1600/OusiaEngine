#pragma once

// 調整用の数値をまとめる。単位はワールド座標（m 相当）と秒
namespace ou::Tuning {

// ----- アリーナ（xy 平面の箱。中心が原点）-----
inline constexpr float kArenaHalfWidth = 16.0f;
inline constexpr float kArenaHalfHeight = 9.0f;

// ----- プレイヤーの移動 -----
inline constexpr float kPlayerRadius = 0.5f;
// アリーナの端から端（32）を約 0.5 秒で横断する
inline constexpr float kPlayerMaxSpeed = 64.0f;
// 速度帯の境目。再発射閾値 > ダメージ閾値
inline constexpr float kRelaunchSpeed = 24.0f;
inline constexpr float kDamageSpeed = 10.0f;
// 時間による減速（1 秒で exp(-kPlayerDrag) 倍）
inline constexpr float kPlayerDrag = 0.55f;
// 壁で反射したときに残る速さの割合
inline constexpr float kWallRestitution = 0.93f;
// 低速帯だけにかかる重力
inline constexpr float kGravity = 14.0f;

// ----- 引っ張り（溜め）-----
// 実時間で数える（スローの影響を受けない）
inline constexpr float kMaxChargeTime = 0.8f;
// ちょん押しでも最高速度のこの割合で飛ぶ
inline constexpr float kMinLaunchRatio = 0.35f;
// 溜め中のスロー倍率（ボスと弾にもかかる）
inline constexpr float kChargeTimeScale = 0.25f;
// これ以上の入力量のときだけ向きを拾う
inline constexpr float kInputDirectionThreshold = 0.5f;
// これ未満を「離した（入力 0）」とみなす
inline constexpr float kInputReleaseThreshold = 0.2f;
// 溜めたときにプレイヤーが伸びる最大量
inline constexpr float kMaxStretch = 0.6f;

// ----- ストック -----
inline constexpr int kMaxStock = 3;
inline constexpr float kStockRecoverTime = 1.2f;
// 10 コンボごとにストックを 1 回復する（検討中の仕様。false で無効）
inline constexpr bool kComboRecoversStock = true;
inline constexpr int kComboPerStock = 10;

// ----- プレイヤーの HP と被弾 -----
inline constexpr float kPlayerMaxHp = 100.0f;
// スーパーアーマー中に受けるダメージの割合
inline constexpr float kArmorDamageRatio = 0.4f;
// 被弾の吹っ飛び速度（ダメージ閾値未満にクランプされる）
inline constexpr float kKnockbackSpeed = 9.0f;
inline constexpr float kHitInvincibleTime = 0.8f;
// アーマーで受けたときの無敵（連続ヒットを防ぐだけ）
inline constexpr float kArmorInvincibleTime = 0.15f;

// ----- 攻撃・ヒットストップ・コンボ -----
// ボスへのダメージ = 速さ × この値
inline constexpr float kDamagePerSpeed = 0.5f;
inline constexpr float kWeakPointMultiplier = 3.0f;
// 弱点に当たったとみなす角度（ボスの中心から見た弱点とのずれ）
inline constexpr float kWeakPointHitAngle = 0.6f;
// ボスから跳ね返るときに残る速さの割合
inline constexpr float kBossBounceRestitution = 0.95f;
// 同じボスへの多段ヒットを防ぐ間隔
inline constexpr float kBossHitCooldown = 0.15f;
inline constexpr float kHitStopMin = 0.04f;
inline constexpr float kHitStopMax = 0.12f;
inline constexpr float kComboResetTime = 3.0f;

// ----- カメラ -----
inline constexpr float kCameraFieldOfView = 50.0f;
// 通常はステージの約 2/3 が映る程度
inline constexpr float kCameraNormalViewRatio = 0.66f;
// 全体を映すときの余白
inline constexpr float kCameraFullViewMargin = 1.12f;
inline constexpr float kCameraFollowSharpness = 8.0f;

// ----- 予測線 -----
inline constexpr float kPreviewMaxTime = 1.6f;
inline constexpr float kPreviewStep = 1.0f / 60.0f;

// ----- ボス -----
inline constexpr float kBossRadius = 1.6f;
inline constexpr float kBossPhase1Hp = 300.0f;
inline constexpr float kBossPhase2Hp = 600.0f;
inline constexpr float kBossPhase1Interval = 1.2f;
inline constexpr float kBossPhase2Interval = 0.7f;
inline constexpr float kBossTransitionTime = 2.0f;
inline constexpr float kWeakPointSwitchTime = 6.0f;
inline constexpr float kWeakPointWarnTime = 1.0f;

// ----- 弾 -----
inline constexpr float kSmallBulletRadius = 0.3f;
inline constexpr float kSmallBulletDamage = 8.0f;
inline constexpr float kHomingBulletRadius = 0.4f;
inline constexpr float kHomingBulletDamage = 10.0f;
inline constexpr float kGiantBulletDamage = 20.0f;
inline constexpr int kGiantBulletMaxBounces = 6;
inline constexpr int kGiantBulletMaxCount = 3;
inline constexpr float kDashDamage = 25.0f;
inline constexpr float kAreaAttackDamage = 25.0f;

} // namespace ou::Tuning
