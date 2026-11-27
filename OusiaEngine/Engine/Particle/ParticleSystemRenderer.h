#pragma once

#include "Graphics/GraphicsDevice.h"
#include "Math/Color.h"
#include "Math/Matrix4x4.h"
#include "Math/Vector3.h"
#include "Particle/ParticleSystem.h"
#include "Render/Asset/Shader.h"
#include "Render/Asset/Texture.h"
#include "Render/Renderer.h"

#include <cstdint>
#include <memory>
#include <vector>


namespace ou {

// 前方宣言
class Material;

class ParticleSystemRenderer final: public Renderer {
public: // インナークラス

	enum class BlendMode : uint8_t {
		Additive, // 加算
		Subtract, // 減算
		Multiply, // 乗算
		Alpha,	  // アルファブレンド

		Count,
	};

	enum class SortMode : uint8_t {
		None,			 // ソートなし
		Distance,		 // カメラから遠い順
		OldestInFront,	 // 古いパーティクルを手前に
		YoungestInFront, // 新しいパーティクルを手前に
	};

	// 定数バッファ: b0
	struct PerParticleSystem {
		Matrix4x4 world;
		uint32_t particleBufferIndex;
		uint32_t textureIndex;
		uint32_t blendMode;
	};

	struct ParticleData {
		Vector3 position;
		float size;
		Color color;
	};


public: // 静的メンバ関数

	static void StaticInitialize();
	static void StaticFinalize();


public: // メンバ関数

	void Awake() override;
	void LateUpdate() override;
	void Serialize(Serializer &serializer) override;

	void Draw(std::string_view lightMode, Graphics::CommandList cmd) const override;

	void SetBlendMode(BlendMode blendMode);
	BlendMode GetBlendMode() const;

	void SetSortMode(SortMode sortMode);
	SortMode GetSortMode() const;


protected: // メンバ関数

	void AwakeFromLoad() override;


private: // 静的メンバ変数

	static inline std::shared_ptr<Shader> sShader_;
	static inline std::shared_ptr<Material> sDefaultMaterial_;
	static inline std::shared_ptr<Texture> sDefaultTexture_;


private: // メンバ変数

	ParticleSystem *particleSystem_ = nullptr;

	BlendMode blendMode_ = BlendMode::Additive;
	SortMode sortMode_ = SortMode::None;

	Graphics::GpuBuffer particleBuffers_[Graphics::GraphicsDevice::BUFFER_COUNT];
	uint32_t capacity_ = 0;
	uint32_t particleCount_ = 0;

	// ソート用
	std::vector<uint32_t> drawOrder_;
	std::vector<float> sortDistances_;


private: // メンバ関数

	void EnsureCapacity(uint32_t count);

	void SortByDistance(const std::vector<ParticleSystem::Particle> &particles);
};

} // namespace ou
