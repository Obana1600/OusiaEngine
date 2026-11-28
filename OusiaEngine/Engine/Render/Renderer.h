#pragma once

#include "Graphics/GraphicsTypes.h"
#include "Math/Matrix4x4.h"
#include "Scene/Behavior.h"
#include "Math/Bounds.h"

#include <memory>
#include <string_view>
#include <vector>


namespace ou {

// 前方宣言
class Mesh;
class Material;


enum class ShadowCastingMode : uint32_t {
	Off,
	On,
	TwoSided,	 // 影を描くときだけ裏面も描く
	ShadowsOnly, // 影だけを落とし、物そのものは描かない
};


class Renderer: public Behavior {
	friend class SceneManager;

public: // 定数

	static constexpr uint32_t kNoSkinning = 0xFFFFFFFF;


public: // インナークラス

	// 定数バッファ: b0
	struct PerDraw {
		Matrix4x4 world;
		Matrix4x4 worldInverseTranspose;

		uint32_t boneMatrixBufferIndex = kNoSkinning;
		uint32_t boneWeightBufferIndex = kNoSkinning;
		uint32_t receiveShadows;
		uint32_t padding;
	};


public: // メンバ関数

	void Awake() override;
	void OnDestroy() override;
	void Serialize(Serializer &serializer) override;

	virtual void Draw(std::string_view lightMode, Graphics::CommandList cmd) const = 0;
	virtual Bounds GetLocalBounds() const;
	virtual int GetRenderQueue() const;

	Bounds GetBounds() const;

	void SetSharedMaterial(std::shared_ptr<Material> material);
	const std::shared_ptr<Material> &GetSharedMaterial() const;

	void SetSharedMaterials(std::vector<std::shared_ptr<Material>> materials);
	const std::vector<std::shared_ptr<Material>> &GetSharedMaterials() const;

	const std::shared_ptr<Material> &GetMaterial();
	const std::vector<std::shared_ptr<Material>> &GetMaterials();

	void SetShadowCastingMode(ShadowCastingMode shadowCastingMode);
	ShadowCastingMode GetShadowCastingMode() const;

	void SetReceiveShadows(bool receiveShadows);
	bool GetReceiveShadows() const;


protected: // メンバ関数

	PerDraw MakePerDraw() const;
	void DrawMesh(Mesh &mesh, const PerDraw &perDraw, std::string_view lightMode, Graphics::CommandList cmd) const;

	Material *FindMaterialForSubMesh(uint32_t subMeshIndex) const;

	void AwakeFromLoad() override;


private: // メンバ変数

	std::vector<std::shared_ptr<Material>> materials_;
	std::vector<std::shared_ptr<Material>> serializedMaterials_;
	std::vector<bool> isInstanced_;

	ShadowCastingMode shadowCastingMode_ = ShadowCastingMode::On;
	bool receiveShadows_ = true;


private: // メンバ関数

	// 複製したマテリアルを None にしたもの（複製は保存できない）
	std::vector<std::shared_ptr<Material>> MakeAssetMaterials() const;

	void InstantiateMaterial(size_t index);

	virtual bool CanCullWithBounds() const;
};

} // namespace ou
