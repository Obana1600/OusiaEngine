#pragma once

#include "Math/Vector2.h"
#include "Math/Vector3.h"
#include <Windows.h>
#include <cstdint>
#include <d3d12.h>
#include <string>
#include <vector>
#include <wrl.h>


namespace ou {

class Material;

class Mesh {
public: // インナークラス

	struct VertexInfo {
		Vector3 pos;
		Vector3 normal;
		Vector2 uv;
	};


public: // メンバ関数

	void Draw(ID3D12GraphicsCommandList *commandList, UINT rootParameterIndexMaterial, UINT rootParameterIndexTexture);
	void Draw(ID3D12GraphicsCommandList *commandList, UINT rootParameterIndexMaterial, UINT rootParameterIndexTexture, uint32_t textureHandle);

	void AddVertex(const VertexInfo &vertex);

	void AddIndex(uint32_t index);

	size_t GetVertexCount() const;

	void CreateBuffers();


	const std::string &GetName() const {
		return name_;
	}

	void SetName(const std::string &name) {
		name_ = name;
	}

	Material *GetMaterial() const {
		return material_;
	}

	void SetMaterial(Material *material);

	const D3D12_VERTEX_BUFFER_VIEW &GetVBV() const {
		return vertexBufferView_;
	}

	const D3D12_INDEX_BUFFER_VIEW &GetIBV() const {
		return indexBufferView_;
	}

	const std::vector<VertexInfo> &GetVertices() const {
		return vertices_;
	}

	const std::vector<uint32_t> &GetIndices() const {
		return indices_;
	}


private: // メンバ変数

	std::string name_;

	Microsoft::WRL::ComPtr<ID3D12Resource> vertexResource_;
	Microsoft::WRL::ComPtr<ID3D12Resource> indexResource_;
	D3D12_VERTEX_BUFFER_VIEW vertexBufferView_{};
	D3D12_INDEX_BUFFER_VIEW indexBufferView_{};

	std::vector<VertexInfo> vertices_;
	std::vector<uint32_t> indices_;

	Material *material_ = nullptr;
};

} // namespace ou