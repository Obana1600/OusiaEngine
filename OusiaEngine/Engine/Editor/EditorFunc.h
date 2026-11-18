#pragma once

#include "Graphics/GraphicsTypes.h"
#include "Math/Vector2.h"

#include <cstdint>


namespace ou {

uint64_t ToTextureId(const Graphics::Texture &texture);

// Game ビュー・Scene ビューで描く大きさ。PlayerSettings の Default Screen Size の比で、available に収まるいちばん大きなもの
Vector2 FitToPlayerAspect(const Vector2 &available);

// size の絵を、今の位置から available の真ん中に置く（余りは帯になる）
void CenterNextItem(const Vector2 &available, const Vector2 &size);

} // namespace ou
