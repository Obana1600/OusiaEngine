#pragma once


namespace ou {

// 前方宣言
class GameObject;
class Scene;

// Unity の GameObject メニュー（Hierarchy の右クリックとメインメニューで同じものを出す）
// Create Empty・3D Object・Camera・Light・UI のテンプレートを作る
// エディタの画面からだけ使う（ゲームのコードからは見えない）
class GameObjectMenu final {
public: // 静的メンバ関数

	GameObjectMenu() = delete;


	// メニューの項目を出す。選んだら作って選択し、シーンを変更ありにする（parent が nullptr ならルートに作る）
	static void DrawItems(Scene *scene, GameObject *parent);
};

} // namespace ou
