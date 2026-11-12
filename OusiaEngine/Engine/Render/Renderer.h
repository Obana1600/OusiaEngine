#pragma once

#include "Graphics/GraphicsTypes.h"
#include "Math/Matrix4x4.h"
#include "Scene/Behavior.h"

#include <memory>
#include <vector>


namespace ou {

// 前方宣言
class Mesh;
class Material;

class Renderer: public Behavior {
public: // 定数

	static constexpr uint32_t kNoSkinning = 0xFFFFFFFF;


public: // インナークラス

	// 定数バッファ: b0
	struct PerDraw {
		Matrix4x4 world;
		Matrix4x4 worldInverseTranspose;

		uint32_t boneMatrixBufferIndex = kNoSkinning;
		uint32_t boneWeightBufferIndex = kNoSkinning;
		uint32_t padding[2];
	};


public: // メンバ関数

	void Awake() override;
	void OnDestroy() override;
	void Serialize(Serializer &serializer) override;

	virtual void Draw(Graphics::CommandList cmd) const = 0;

	virtual int GetRenderQueue() const;

	void SetSharedMaterial(std::shared_ptr<Material> material);
	const std::shared_ptr<Material> &GetSharedMaterial() const;

	void SetSharedMaterials(std::vector<std::shared_ptr<Material>> materials);
	const std::vector<std::shared_ptr<Material>> &GetSharedMaterials() const;

	const std::shared_ptr<Material> &GetMaterial();
	const std::vector<std::shared_ptr<Material>> &GetMaterials();


protected: // メンバ関数

	PerDraw MakePerDraw() const;
	void DrawMesh(Mesh &mesh, const PerDraw &perDraw, Graphics::CommandList cmd) const;

	Material *FindMaterialForSubMesh(uint32_t subMeshIndex) const;

	void SerializeAssetMaterials(Serializer &serializer);


private: // メンバ変数

	std::vector<std::shared_ptr<Material>> materials_;
	std::vector<bool> isInstanced_;


private: // メンバ関数

	void InstantiateMaterial(size_t index);
};

} // namespace ou
