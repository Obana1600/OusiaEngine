#pragma once


namespace ou {

// 前方宣言
class Scene;

// アリーナの見た目（壁と床）。当たり判定は Motion2D が Tuning の大きさで行う
void CreateArena(Scene &scene);

} // namespace ou
