#pragma once

#include "Math/Color.h"
#include "Scene/Component.h"

#include <cstdint>
#include <memory>


namespace ou {

// 前方宣言
class Material;
class MeshRenderer;

// Ricochet の範囲表示に使うリング
// Unity の Image の Filled（Radial360）と同じ名前の設定で、xy 平面に扇形のリングを描く。
// 設定が変わったフレームだけ、扇形のメッシュを作り直して同じ GameObject の MeshRenderer に渡す
class FilledRing final: public Component {
public: // インナークラス

	// Unity の Image.Origin360 と同じ
	enum class Origin360 : uint8_t {
		Bottom,
		Right,
		Top,
		Left,
	};


public: // メンバ関数

	void Awake() override;
	void LateUpdate() override;

	// 0 で何も出ず、1 で円が一周する
	void SetFillAmount(float fillAmount);
	float GetFillAmount() const;

	// true でカメラ（-z 側）から見て時計回りに満ちる
	void SetFillClockwise(bool fillClockwise);
	bool GetFillClockwise() const;

	void SetFillOrigin(Origin360 fillOrigin);
	Origin360 GetFillOrigin() const;

	// innerRadius を 0 にすると円盤になる
	void SetInnerRadius(float innerRadius);
	float GetInnerRadius() const;

	void SetOuterRadius(float outerRadius);
	float GetOuterRadius() const;

	void SetColor(const Color &color);
	const Color &GetColor() const;

	// 一周ぶんの分割数
	void SetSegments(uint32_t segments);
	uint32_t GetSegments() const;


private: // メンバ変数

	float fillAmount_ = 1.0f;
	bool fillClockwise_ = true;
	Origin360 fillOrigin_ = Origin360::Top;
	float innerRadius_ = 0.8f;
	float outerRadius_ = 1.0f;
	uint32_t segments_ = 64;
	Color color_ = Color::White;

	MeshRenderer *meshRenderer_ = nullptr;
	std::shared_ptr<Material> material_;
	bool isDirty_ = true;


private: // メンバ関数

	void RebuildMesh();
};

} // namespace ou
