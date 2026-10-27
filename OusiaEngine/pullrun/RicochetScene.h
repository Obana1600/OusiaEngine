#pragma once


namespace ou {

// 前方宣言
class Scene;

inline constexpr const char *kRicochetSceneName = "Ricochet";

// ボス戦のシーンを組み立てる。コンポーネントの Update は作った順に呼ばれるので、移動 → 当たり判定 → 表示の順に作る
void BuildRicochetScene(Scene &scene);

// ステージを照らす平行光（主光と、反対側からの弱い光）。タイトルの背景でも使う
void CreateStageLights(Scene &scene);

} // namespace ou
