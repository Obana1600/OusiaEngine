#pragma once


namespace ou {

// 前方宣言
class Scene;

inline constexpr const char *kTitleSceneName = "Title";

// タイトルのシーンを組み立てる。背景はボス戦と同じステージと集中線
void BuildTitleScene(Scene &scene);

} // namespace ou
