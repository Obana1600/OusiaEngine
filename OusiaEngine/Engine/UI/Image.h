#pragma once

#include "Math/Rect.h"
#include "Math/Vector2.h"
#include "UI/Graphic.h"

#include <memory>


namespace ou {

// 前方宣言
class Sprite;
class Texture;

class Image final: public Graphic {
public: // メンバ関数

	void Serialize(Serializer &serializer) override;

	void SetSprite(std::shared_ptr<Sprite> sprite);
	const std::shared_ptr<Sprite> &GetSprite() const;

	void SetNativeSize();

	const std::shared_ptr<Texture> &GetMainTexture() const override;


protected: // メンバ関数

	void AwakeFromLoad() override;

	void OnPopulateMesh(VertexHelper &vh) override;


private: // メンバ変数

	std::shared_ptr<Sprite> sprite_;

	// Sprite の中身（保存する形）。違えば AwakeFromLoad で Sprite を作り直す
	std::shared_ptr<Texture> spriteTexture_;
	Rect spriteRect_ = Rect::Zero;
	Vector2 spritePivot_ = {0.5f, 0.5f};
};

} // namespace ou
