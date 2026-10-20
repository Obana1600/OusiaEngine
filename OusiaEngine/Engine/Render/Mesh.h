#pragma once

#include "Core/Object.h"
#include "Graphics/GraphicsTypes.h"
#include "Math/Vector2.h"
#include "Math/Vector3.h"

#include <cstdint>
#include <memory>
#include <vector>


namespace ou {

class Mesh final: public Object {
public: // インナークラス

	struct Vertex {
		Vector3 position;
		Vector3 normal;
		Vector2 texcoord;
	};

	struct SubMesh {
		uint32_t indexStart = 0;
		uint32_t indexCount = 0;
	};


public: // 静的メンバ関数

	static std::shared_ptr<Mesh> CreateCube();

	static const Graphics::InputLayout &GetInputLayout();


public: // メンバ関数

	void Upload();

	void SetVertices(std::vector<Vertex> vertices);
	const std::vector<Vertex> &GetVertices() const;

	void SetIndices(std::vector<uint32_t> indices);
	const std::vector<uint32_t> &GetIndices() const;

	void SetSubMeshes(std::vector<SubMesh> subMeshes);
	const std::vector<SubMesh> &GetSubMeshes() const;

	void BindBuffers(Graphics::CommandList cmd);
	void DrawSubMesh(uint32_t subMeshIndex, Graphics::CommandList cmd) const;


private: // メンバ変数

	std::vector<Vertex> vertices_;
	std::vector<uint32_t> indices_;
	std::vector<SubMesh> subMeshes_;

	Graphics::GpuBuffer vertexBuffer_;
	Graphics::GpuBuffer indexBuffer_;
};

} // namespace ou
