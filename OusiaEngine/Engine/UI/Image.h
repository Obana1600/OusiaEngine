#pragma once

#include "UI/Graphic.h"

#include <memory>


namespace ou {

// 前方宣言
class Sprite;

class Image final: public Graphic {
public: // メンバ関数

	void SetSprite(std::shared_ptr<Sprite> sprite);
	const std::shared_ptr<Sprite> &GetSprite() const;

	void SetNativeSize();

	void Draw(Graphics::CommandList cmd, const Matrix4x4 &viewProj) const override;


private: // メンバ変数

	std::shared_ptr<Sprite> sprite_;
};

} // namespace ou
