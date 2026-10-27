#pragma once

#include "Math/Color.h"
#include "Math/Vector3.h"
#include "Scene/Component.h"

#include <memory>
#include <string_view>


namespace ou {

// 前方宣言
class Light;
class Material;
class Transform;
class Mesh;
class Scene;
class MeshRenderer;
class Texture;

// ゲームで共有するメッシュ。シーンが持つので、エンジンの終了より先に解放される
class GameAssets final: public Component {
public: // 静的メンバ関数

	static GameAssets *GetInstance() { return sInstance_; }


public: // メンバ関数

	void Awake() override;
	void OnDestroy() override;

	const std::shared_ptr<Mesh> &GetSphere() const { return sphere_; }
	const std::shared_ptr<Mesh> &GetCube() const { return cube_; }

	// パーティクルのテクスチャ：大きくぼけた丸・小さな丸・ひし形
	const std::shared_ptr<Texture> &GetGlowTexture() const { return glowTexture_; }
	const std::shared_ptr<Texture> &GetDotTexture() const { return dotTexture_; }
	const std::shared_ptr<Texture> &GetDiamondTexture() const { return diamondTexture_; }


private: // 静的メンバ変数

	static inline GameAssets *sInstance_ = nullptr;


private: // メンバ変数

	std::shared_ptr<Mesh> sphere_;
	std::shared_ptr<Mesh> cube_;
	std::shared_ptr<Texture> glowTexture_;
	std::shared_ptr<Texture> dotTexture_;
	std::shared_ptr<Texture> diamondTexture_;
};

// 単色のメッシュを持つ GameObject を作る。unlit はライトの影響を受けない（弾など、はっきり見せたいもの）
MeshRenderer *CreateMeshObject(Scene &scene, std::string_view name, const std::shared_ptr<Mesh> &mesh,
							   const Vector3 &position, const Vector3 &scale, const Color &color, bool unlit);

// 半透明・ライトなしのマテリアル（予兆の表示など）
std::shared_ptr<Material> CreateTransparentMaterial(const Color &color);

// parent の中心に置く点光源。自分の表面は内側から照らされない（裏から当たる）ので、周りと床だけが明るくなる
Light *CreatePointLight(Scene &scene, Transform *parent, const Color &color, float intensity, float range);

} // namespace ou
