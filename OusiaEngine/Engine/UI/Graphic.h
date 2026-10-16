#pragma once

#include "Asset/Shader.h"
#include "Asset/Texture.h"
#include "Graphics/GraphicsTypes.h"
#include "Math/Color.h"
#include "Math/Matrix4x4.h"
#include "Math/Vector4.h"
#include "Scene/Component.h"

#include <cstdint>


namespace ou {

// 前方宣言
class RectTransform;

class Graphic: public Component {
public: // インナークラス

	// 定数バッファ: b0
	struct PerGraphic {
		Matrix4x4 worldViewProj;
		Matrix4x4 uvTransform;
		Color color;
		Vector4 positionRect;
		Vector4 texcoordRect;
		uint32_t textureIndex;
	};


public: // 静的メンバ関数

	static void StaticInitialize(const Graphics::RenderPassInfo &renderPass);
	static void StaticFinalize();

	static void BindPipelineState(Graphics::CommandList cmd);


public: // メンバ関数

	void Awake() override;

	void SetColor(const Color &color);
	const Color &GetColor() const;

	virtual void Draw(Graphics::CommandList cmd, const Matrix4x4 &viewProj) const = 0;


protected: // 静的メンバ関数

	static const std::shared_ptr<Texture> &GetWhiteTexture();


protected: // メンバ関数

	RectTransform *GetRectTransform() const;


private: // メンバ変数

	Color color_ = Color::White;
	RectTransform *rectTransform_ = nullptr;


private: // 静的メンバ変数

	static inline std::shared_ptr<Shader> sVs_;
	static inline std::shared_ptr<Shader> sPs_;
	static inline Graphics::PipelineState sPso_{};
	static inline std::shared_ptr<Texture> sWhiteTexture_;
};

} // namespace ou
