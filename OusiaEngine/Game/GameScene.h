#pragma once

#include "Graphics/GraphicsTypes.h"
#include "Scene/GameObject.h"
#include "Scene/Scene.h"

#include <memory>


namespace ou {

// 前方宣言
class ParticleSystem;
class ParticleSystemRenderer;
class Texture;

class GameScene {
public: // メンバ関数

	void Initialize();

	void Update();

	void Draw(Graphics::CommandList cmd) const;
	void DrawOverlay(Graphics::CommandList cmd) const;


private: // メンバ変数

	std::unique_ptr<Scene> scene_;

	GameObject *emitter_ = nullptr;
	ParticleSystem *particleSystem_ = nullptr;
	ParticleSystemRenderer *particleRenderer_ = nullptr;

	// エミッタを円を描くように動かす（Local と World の違いを見るため）
	bool isEmitterMoving_ = false;

	// ImGui で選べるテクスチャ（nullptr は白）
	std::shared_ptr<Texture> uvCheckerTexture_;
	std::shared_ptr<Texture> softCircleTexture_;
	int textureIndex_ = 2;


private: // メンバ関数

	void UpdateEmitterMotion();

	// パーティクルの設定を ImGui で操作する
	void ShowParticleWindow();
};

} // namespace ou
