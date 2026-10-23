#pragma once

#include "Graphics/GraphicsTypes.h"
#include "Math/Color.h"
#include "Math/Matrix4x4.h"
#include "Math/Vector4.h"
#include "Render/Asset/Material.h"
#include "Render/Asset/Texture.h"
#include "Scene/Behaviour.h"

#include <cstdint>
#include <memory>


namespace ou {

// 前方宣言
class RectTransform;

class Graphic: public Behaviour {
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

	static void StaticInitialize();
	static void StaticFinalize();

	static const std::shared_ptr<Material> &GetDefaultGraphicMaterial();


public: // メンバ関数

	void Awake() override;

	void SetMaterial(std::shared_ptr<Material> material);
	const std::shared_ptr<Material> &GetMaterial() const;
	virtual const std::shared_ptr<Material> &GetDefaultMaterial() const;

	void SetColor(const Color &color);
	const Color &GetColor() const;

	virtual void Draw(Graphics::CommandList cmd, const Matrix4x4 &viewProj) const = 0;


protected: // 静的メンバ関数

	static const std::shared_ptr<Texture> &GetWhiteTexture();


protected: // メンバ関数

	RectTransform *GetRectTransform() const;

	void DrawWithMaterial(Graphics::CommandList cmd, uint32_t vertexCount, uint32_t instanceCount = 1) const;


private: // メンバ変数

	std::shared_ptr<Material> material_;
	Color color_ = Color::White;
	RectTransform *rectTransform_ = nullptr;


private: // 静的メンバ変数

	static inline std::shared_ptr<Material> sDefaultMaterial_;
	static inline std::shared_ptr<Texture> sWhiteTexture_;
};

} // namespace ou
