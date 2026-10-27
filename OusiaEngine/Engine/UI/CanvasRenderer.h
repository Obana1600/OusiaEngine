#pragma once

#include "Math/Color.h"
#include "Scene/Component.h"
#include "UI/UIVertex.h"

#include <cstdint>
#include <memory>
#include <vector>


namespace ou {

// 前方宣言
class Material;
class Texture;

class CanvasRenderer final: public Component {
public: // インナークラス

	struct SubMesh {
		uint32_t indexStart = 0;
		uint32_t indexCount = 0;
	};


public: // メンバ関数

	void SetColor(const Color &color);
	const Color &GetColor() const;

	void SetAlpha(float alpha);
	float GetAlpha() const;

	void SetMesh(std::vector<UIVertex> vertices, std::vector<uint32_t> indices, std::vector<SubMesh> subMeshes = {});
	void Clear();

	const std::vector<UIVertex> &GetVertices() const;
	const std::vector<uint32_t> &GetIndices() const;
	const std::vector<SubMesh> &GetSubMeshes() const;

	void SetMaterialCount(uint32_t materialCount);
	uint32_t GetMaterialCount() const;

	void SetMaterial(std::shared_ptr<Material> material, uint32_t index);
	const std::shared_ptr<Material> &GetMaterial(uint32_t index) const;

	void SetTexture(std::shared_ptr<Texture> texture);
	const std::shared_ptr<Texture> &GetTexture() const;


private: // メンバ変数

	Color color_ = Color::White;

	std::vector<UIVertex> vertices_;
	std::vector<uint32_t> indices_;
	std::vector<SubMesh> subMeshes_;

	std::vector<std::shared_ptr<Material>> materials_;
	std::shared_ptr<Texture> texture_;
};

} // namespace ou
