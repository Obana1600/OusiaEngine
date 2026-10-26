#pragma once

#include "Graphics/GraphicsDevice.h"
#include "Math/Color.h"
#include "Math/Vector3.h"
#include "Render/Asset/Shader.h"
#include "Render/Asset/Texture.h"
#include "Render/Renderer.h"

#include <cstdint>
#include <memory>
#include <span>
#include <vector>


namespace ou {

// 前方宣言
class Material;

class LineRenderer final: public Renderer {
public: // インナークラス

	// 定数バッファ: b0
	struct PerLineRenderer {
		uint32_t pointBufferIndex;
		uint32_t pointCount;
		uint32_t textureIndex;
		uint32_t loop;
	};

	struct PointData {
		Vector3 position;
		float width;
		Color color;
		float texcoord;
		float padding[3];
	};


public: // 静的メンバ関数

	static void StaticInitialize();
	static void StaticFinalize();


public: // メンバ関数

	void Awake() override;
	void LateUpdate() override;

	void Draw(Graphics::CommandList cmd) const override;

	void SetPositionCount(uint32_t count);
	uint32_t GetPositionCount() const;

	void SetPosition(uint32_t index, const Vector3 &position);
	Vector3 GetPosition(uint32_t index) const;

	void SetPositions(std::span<const Vector3> positions);
	const std::vector<Vector3> &GetPositions() const;

	void SetStartWidth(float width);
	float GetStartWidth() const;

	void SetEndWidth(float width);
	float GetEndWidth() const;

	void SetWidthMultiplier(float widthMultiplier);
	float GetWidthMultiplier() const;

	void SetStartColor(const Color &color);
	const Color &GetStartColor() const;

	void SetEndColor(const Color &color);
	const Color &GetEndColor() const;

	void SetUseWorldSpace(bool useWorldSpace);
	bool GetUseWorldSpace() const;

	void SetLoop(bool loop);
	bool GetLoop() const;


private: // 静的メンバ変数

	static inline std::shared_ptr<Shader> sShader_;
	static inline std::shared_ptr<Material> sDefaultMaterial_;
	static inline std::shared_ptr<Texture> sWhiteTexture_;


private: // メンバ変数

	std::vector<Vector3> positions_;

	float startWidth_ = 1.0f;
	float endWidth_ = 1.0f;
	float widthMultiplier_ = 1.0f;
	Color startColor_ = Color::White;
	Color endColor_ = Color::White;
	bool useWorldSpace_ = true;
	bool loop_ = false;

	Graphics::GpuBuffer pointBuffers_[Graphics::GraphicsDevice::BUFFER_COUNT];
	uint32_t capacity_ = 0;
	uint32_t pointCount_ = 0;

	std::vector<Vector3> worldPositions_;
	std::vector<float> distances_;


private: // メンバ関数

	void EnsureCapacity(uint32_t count);
};

} // namespace ou
