#include "VertexHelper.h"

using namespace ou;


void ou::VertexHelper::Clear() {
	vertices_.clear();
	indices_.clear();
}


uint32_t ou::VertexHelper::GetCurrentVertCount() const {
	return static_cast<uint32_t>(vertices_.size());
}


uint32_t ou::VertexHelper::GetCurrentIndexCount() const {
	return static_cast<uint32_t>(indices_.size());
}


void ou::VertexHelper::AddVert(const UIVertex &vertex) {
	vertices_.push_back(vertex);
}


void ou::VertexHelper::AddVert(const Vector3 &position, const Color &color, const Vector4 &uv0) {
	vertices_.push_back({.position = position,
						 .color = color,
						 .uv0 = uv0});
}


void ou::VertexHelper::AddTriangle(uint32_t index0, uint32_t index1, uint32_t index2) {
	indices_.push_back(index0);
	indices_.push_back(index1);
	indices_.push_back(index2);
}


void ou::VertexHelper::AddUIVertexQuad(std::span<const UIVertex, 4> vertices) {
	const auto start = GetCurrentVertCount();
	for (const auto &vertex : vertices) {
		AddVert(vertex);
	}

	AddTriangle(start, start + 1, start + 2);
	AddTriangle(start + 2, start + 3, start);
}


void ou::VertexHelper::FillMesh(std::vector<UIVertex> &vertices, std::vector<uint32_t> &indices) const {
	vertices = vertices_;
	indices = indices_;
}
