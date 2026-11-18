#pragma once

#include "Math/Color.h"
#include "Render/Asset/Material.h"
#include "Render/Asset/Texture.h"
#include "Scene/Behavior.h"

#include <memory>


namespace ou {

// 前方宣言
class CanvasRenderer;
class RectTransform;
class VertexHelper;

class Graphic: public Behavior {
public: // 静的メンバ関数

	static void StaticInitialize();
	static void StaticFinalize();

	static const std::shared_ptr<Material> &GetDefaultGraphicMaterial();


public: // メンバ関数

	void Awake() override;
	void Update() override;
	void OnEnable() override;
	void OnDisable() override;
	void OnDestroy() override;
	void Serialize(Serializer &serializer) override;

	virtual const std::shared_ptr<Material> &GetDefaultMaterial() const;
	virtual const std::shared_ptr<Texture> &GetMainTexture() const;

	void SetMaterial(std::shared_ptr<Material> material);
	const std::shared_ptr<Material> &GetMaterial() const;

	void SetColor(const Color &color);
	const Color &GetColor() const;

	void SetRaycastTarget(bool raycastTarget);
	bool GetRaycastTarget() const;

	void CrossFadeColor(const Color &targetColor, float duration, bool ignoreTimeScale, bool useAlpha);

	CanvasRenderer *GetCanvasRenderer() const;

	void SetAllDirty();
	void SetVerticesDirty();
	void SetMaterialDirty();

	void Rebuild();

	void OnRectTransformDimensionsChange();


protected: // 静的メンバ関数

	static const std::shared_ptr<Texture> &GetWhiteTexture();


protected: // メンバ関数

	void AwakeFromLoad() override;

	RectTransform *GetRectTransform() const;

	virtual void UpdateGeometry();
	virtual void UpdateMaterial();

	virtual void OnPopulateMesh(VertexHelper &vh);


private: // インナークラス

	// TODO: Twenner を作る 
	struct ColorTween {
		Color startColor = Color::White;
		Color targetColor = Color::White;
		float duration = 0.0f;
		float elapsed = 0.0f;
		bool ignoreTimeScale = false;
	};


private: // メンバ変数

	std::shared_ptr<Material> material_;
	Color color_ = Color::White;
	RectTransform *rectTransform_ = nullptr;
	CanvasRenderer *canvasRenderer_ = nullptr;

	bool raycastTarget_ = true;

	bool isVerticesDirty_ = false;
	bool isMaterialDirty_ = false;

	ColorTween colorTween_{};


private: // 静的メンバ変数

	static inline std::shared_ptr<Material> sDefaultMaterial_;
	static inline std::shared_ptr<Texture> sWhiteTexture_;
};

} // namespace ou
