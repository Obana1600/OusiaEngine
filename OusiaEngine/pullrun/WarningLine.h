#pragma once

#include "Math/Color.h"
#include "Math/Vector2.h"

#include <span>


namespace ou {

// 前方宣言
class LineRenderer;
class Scene;

// ボスの移動先・突進の軌道を示す予兆線（LineRenderer を持つ GameObject）
class WarningLine {
public: // メンバ関数

	WarningLine() = default;
	WarningLine(const WarningLine &) = delete;
	WarningLine &operator=(const WarningLine &) = delete;
	~WarningLine();

	void Create(Scene &scene, float width, const Color &color);
	void SetPoints(std::span<const Vector2> points);
	// 0〜1 の明るさで点滅させる
	void SetAlpha(float alpha);
	void Destroy();


private: // メンバ変数

	LineRenderer *line_ = nullptr;
	Color color_ = Color::White;
};

} // namespace ou
