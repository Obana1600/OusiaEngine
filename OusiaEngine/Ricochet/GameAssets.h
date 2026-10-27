#pragma once

#include "Math/Color.h"
#include "Math/Vector3.h"
#include "Scene/Component.h"

#include <memory>
#include <string_view>


namespace ou {

// 前方宣言
class Material;
class Mesh;
class Scene;
class MeshRenderer;

// ゲームで共有するメッシュ。シーンが持つので、エンジンの終了より先に解放される
class GameAssets final: public Component {
public: // 静的メンバ関数

	static GameAssets *GetInstance() { return sInstance_; }


public: // メンバ関数

	void Awake() override;
	void OnDestroy() override;

	const std::shared_ptr<Mesh> &GetSphere() const { return sphere_; }
	const std::shared_ptr<Mesh> &GetCube() const { return cube_; }


private: // 静的メンバ変数

	static inline GameAssets *sInstance_ = nullptr;


private: // メンバ変数

	std::shared_ptr<Mesh> sphere_;
	std::shared_ptr<Mesh> cube_;
};

// 単色のメッシュを持つ GameObject を作る。unlit はライトの影響を受けない（弾など、はっきり見せたいもの）
MeshRenderer *CreateMeshObject(Scene &scene, std::string_view name, const std::shared_ptr<Mesh> &mesh,
							   const Vector3 &position, const Vector3 &scale, const Color &color, bool unlit);

// 半透明・ライトなしのマテリアル（予兆の表示など）
std::shared_ptr<Material> CreateTransparentMaterial(const Color &color);

} // namespace ou
