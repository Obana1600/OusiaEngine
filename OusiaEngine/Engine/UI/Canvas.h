#pragma once

#include "Graphics/GraphicsTypes.h"
#include "Math/Color.h"
#include "Math/Matrix4x4.h"
#include "Scene/Behavior.h"

#include <cstdint>
#include <vector>


namespace ou {

// 前方宣言
class CanvasRenderer;
class RectTransform;
class Transform;

class Canvas final: public Behavior {
public: // インナークラス

	// 定数バッファ: b0
	struct PerCanvasRenderer {
		Matrix4x4 worldViewProj;
		Color color;
		uint32_t textureIndex;
		uint32_t padding[3];
	};


public: // 静的メンバ関数

	static const Graphics::InputLayout &GetInputLayout();


public: // メンバ関数

	void Awake() override;
	void LateUpdate() override;
	void OnDestroy() override;

	void Render(Graphics::CommandList cmd) const;


private: // メンバ変数

	RectTransform *rectTransform_ = nullptr;


private: // メンバ関数

	void FitToScreen();

	static void CollectCanvasRenderers(const Transform *transform, std::vector<const CanvasRenderer *> &canvasRenderers);

	static void DrawCanvasRenderer(const CanvasRenderer &canvasRenderer, const Matrix4x4 &viewProj, Graphics::CommandList cmd);
};

} // namespace ou
