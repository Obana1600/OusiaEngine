#pragma once

#include "Transform.h"
#include "pso/PipelineState.h"
#include "pso/RootSignature.h"
#include "pso/VertexBuffer.h"
#include "pso/IndexBuffer.h"
#include "pso/ConstBuffer.h"
#include "3d/DebugCamera.h"
#include <cstdint>
#include <d3d12.h>
#include <memory>
#include <vector>


class GameScene {
public: // メンバ関数

	void Initialize();

	void Update();

	void Draw() const;


private: // メンバ変数

	// pso
	std::unique_ptr<PipelineState> m_pipelineState;
	std::unique_ptr<RootSignature> m_rootSignature;

	// camera
	std::unique_ptr<DebugCamera> m_debugCamera;

	// directionLight
	std::unique_ptr<ConstBuffer> m_dirLightBuffer;

	// texture
	static inline const int kNumTexture = 2;
	uint32_t m_texHandles[kNumTexture] = { 0 };

	// 三角形
	struct TriangleObj {
		std::unique_ptr<ConstBuffer> wvpBuffer;
		std::unique_ptr<ConstBuffer> materialBuffer;
		Transform transform = { { 1.0f, 1.0f, 1.0f }, { 0.0f, 0.0f, 0.0f }, { 0.0f, 0.0f, 0.0f } };
		Transform uvTransform = { { 1.0f, 1.0f, 1.0f }, { 0.0f, 0.0f, 0.0f }, { 0.0f, 0.0f, 0.0f } };
		uint32_t texHandle = 0;
		int texIndex = 0;
	};

	std::vector<TriangleObj> m_triangles;
	std::unique_ptr<VertexBuffer> m_vertexBuffer;
	std::unique_ptr<IndexBuffer> m_indexBuffer;


private: // メンバ関数

	void CreateTriangle();

	void DeleteTriangle(int index);

};