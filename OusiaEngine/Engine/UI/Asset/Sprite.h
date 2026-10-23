#pragma once

#include "Core/Object.h"
#include "Math/Rect.h"
#include "Math/Vector2.h"

#include <memory>


namespace ou {

// 前方宣言
class Texture;

class Sprite final: public Object {
private: // パスキー

	struct Passkey {
		explicit Passkey() = default;
	};


public: // 静的メンバ関数

	static std::shared_ptr<Sprite> Create(std::shared_ptr<Texture> texture);

	static std::shared_ptr<Sprite> Create(std::shared_ptr<Texture> texture,
										  const Rect &rect,
										  const Vector2 &pivot = {0.5f, 0.5f});


public: // メンバ関数

	Sprite(Passkey,
		   std::shared_ptr<Texture> texture,
		   const Rect &rect,
		   const Vector2 &pivot);

	const std::shared_ptr<Texture> &GetTexture() const;
	const Rect &GetRect() const;
	const Vector2 &GetPivot() const;

	Rect MakeUvRect() const;


private: // メンバ変数

	std::shared_ptr<Texture> texture_;
	Rect rect_{};
	Vector2 pivot_{};
};

} // namespace ou