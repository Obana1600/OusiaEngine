#pragma once

#include "Graphics/GraphicsDevice.h"
#include "Math/Matrix4x4.h"
#include "Render/Renderer.h"

#include <cstdint>
#include <memory>
#include <string>
#include <vector>


namespace ou {

// 前方宣言
class Mesh;
class Transform;

class SkinnedMeshRenderer final: public Renderer {
public: // メンバ関数

	void Serialize(Serializer &serializer) override;

	void Draw(Graphics::CommandList cmd) const override;

	void SetMesh(std::shared_ptr<Mesh> mesh);
	const std::shared_ptr<Mesh> &GetMesh() const;

	void SetBones(Transform *root, std::vector<std::string> bonePaths);
	const std::vector<Matrix4x4> &GetBoneMatrices() const;


protected: // メンバ関数

	void AwakeFromLoad() override;


private: // 定数

	static constexpr uint64_t kStaleBoneGeneration = ~0ull;


private: // メンバ変数

	std::shared_ptr<Mesh> mesh_;

	Transform *boneRoot_ = nullptr;
	std::vector<std::string> bonePaths_;

	mutable std::vector<Transform *> bones_;
	mutable std::vector<Matrix4x4> boneMatrices_;
	mutable uint64_t boneGeneration_ = 0;

	Graphics::GpuBuffer boneMatrixBuffers_[Graphics::GraphicsDevice::BUFFER_COUNT];


private: // メンバ関数

	void FindBones() const;
	void UpdateBoneMatrices() const;
};

} // namespace ou
