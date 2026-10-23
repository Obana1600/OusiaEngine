#pragma once


namespace ou {

// 前方宣言
class Scene;

class SceneState {
public: // メンバ関数

	virtual ~SceneState() = default;

	virtual void Initialize(Scene &scene) = 0;

	virtual void Update() {}

	virtual void Finalize() {}
};

} // namespace ou
