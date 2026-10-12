#pragma once

#include "Graphics/GraphicsTypes.h"
#include "Math/Matrix4x4.h"
#include "Scene/Component.h"

#include <memory>
#include <vector>


namespace ou {

// 前方宣言
class Material;

class Renderer: public Component {
public: // インナークラス

	// 定数バッファ: b0
	struct PerDraw {
		Matrix4x4 world;
		Matrix4x4 worldInverseTranspose;
	};


public: // メンバ関数

	void Awake() override;
	void OnDestroy() override;

	virtual void Draw(CommandList cmd) const = 0;

	void SetMaterial(std::shared_ptr<Material> material);
	const std::shared_ptr<Material> &GetMaterial() const;

	void SetMaterials(std::vector<std::shared_ptr<Material>> materials);
	const std::vector<std::shared_ptr<Material>> &GetMaterials() const;


protected: // メンバ関数

	PerDraw MakePerDraw() const;

	Material *FindMaterialForSubMesh(uint32_t subMeshIndex) const;


private: // メンバ変数

	std::vector<std::shared_ptr<Material>> materials_;
};

} // namespace ou
