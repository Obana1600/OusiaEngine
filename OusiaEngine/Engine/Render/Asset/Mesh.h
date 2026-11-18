#pragma once

#include "Core/Object.h"
#include "Graphics/GraphicsTypes.h"
#include "Math/Bounds.h"
#include "Math/Vector2.h"
#include "Math/Vector3.h"
#include "Math/Matrix4x4.h"

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

	struct BoneWeight {
		uint32_t boneIndices[4] = {0, 0, 0, 0};
		float weights[4] = {1.0f, 0.0f, 0.0f, 0.0f};
	};


public: // 静的メンバ関数

	static std::shared_ptr<Mesh> CreateCube();

	static const Graphics::InputLayout &GetInputLayout();


public: // メンバ関数

	void Upload();

	void SetVertices(std::vector<Vertex> vertices);
	const std::vector<Vertex> &GetVertices() const;

	// Unity の Mesh.bounds。頂点を囲む、メッシュの空間での箱（SetVertices で計算する）
	const Bounds &GetBounds() const;

	void SetIndices(std::vector<uint32_t> indices);
	const std::vector<uint32_t> &GetIndices() const;

	void SetSubMeshes(std::vector<SubMesh> subMeshes);
	const std::vector<SubMesh> &GetSubMeshes() const;

	void SetBoneWeights(std::vector<BoneWeight> boneWeights);
	const std::vector<BoneWeight> &GetBoneWeights() const;
	uint32_t GetBoneWeightBufferIndex() const;

	void SetBindPoses(std::vector<Matrix4x4> bindPoses);
	const std::vector<Matrix4x4> &GetBindPoses() const;

	void BindBuffers(Graphics::CommandList cmd);
	void DrawSubMesh(uint32_t subMeshIndex, Graphics::CommandList cmd) const;


private: // メンバ変数

	std::vector<Vertex> vertices_;
	std::vector<uint32_t> indices_;
	std::vector<SubMesh> subMeshes_;
	std::vector<BoneWeight> boneWeights_;
	std::vector<Matrix4x4> bindPoses_;
	Bounds bounds_;

	Graphics::GpuBuffer vertexBuffer_;
	Graphics::GpuBuffer indexBuffer_;
	Graphics::GpuBuffer boneWeightBuffer_;
};

} // namespace ou
