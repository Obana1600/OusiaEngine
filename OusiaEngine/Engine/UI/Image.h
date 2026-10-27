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

	const std::shared_ptr<Texture> &GetMainTexture() const override;


protected: // メンバ関数

	void OnPopulateMesh(VertexHelper &vh) override;


private: // メンバ変数

	std::shared_ptr<Sprite> sprite_;
};

} // namespace ou
