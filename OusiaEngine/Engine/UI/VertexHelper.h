#pragma once

#include "UI/UIVertex.h"

#include <cstdint>
#include <span>
#include <vector>


namespace ou {

class VertexHelper {
public: // メンバ関数

	void Clear();

	uint32_t GetCurrentVertCount() const;
	uint32_t GetCurrentIndexCount() const;

	void AddVert(const UIVertex &vertex);
	void AddVert(const Vector3 &position, const Color &color, const Vector4 &uv0);

	void AddTriangle(uint32_t index0, uint32_t index1, uint32_t index2);

	void AddUIVertexQuad(std::span<const UIVertex, 4> vertices);

	void FillMesh(std::vector<UIVertex> &vertices, std::vector<uint32_t> &indices) const;


private: // メンバ変数

	std::vector<UIVertex> vertices_;
	std::vector<uint32_t> indices_;
};

} // namespace ou
