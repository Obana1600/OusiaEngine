#pragma once

#include "Math/Color.h"
#include "Math/Vector2.h"

#include <memory>
#include <string_view>


namespace ou {

// 前方宣言
class Button;
class EventSystem;
class FontAsset;
class Image;
class RectTransform;
class Scene;
class Text;
class Transform;

// タイトルとリザルトで共通の、メニュー画面の部品

std::shared_ptr<FontAsset> LoadMenuFont();

// GraphicRaycaster 付きの Canvas。ボタンを置く Canvas はこれで作る
Transform *CreateMenuCanvas(Scene &scene);

// EventSystem と StandaloneInputModule（W/S で選び、Space で決める）
EventSystem *CreateEventSystem(Scene &scene);

// 画面全体を覆う Image。下のゲームへのクリックも止める
Image *CreateScreenCover(Scene &scene, Transform *parent, const Color &color);

// 画面の中央を基準に置く文字
Text *CreateMenuText(Scene &scene, Transform *parent, const std::shared_ptr<FontAsset> &font, std::string_view text,
					 const Vector2 &position, float fontSize);

// 画面の中央を基準に置くボタン
Button *CreateMenuButton(Scene &scene, Transform *parent, const std::shared_ptr<FontAsset> &font, std::string_view label,
						 const Vector2 &position);

// 画面の中央を基準に置く（anchor・pivot は中央）
RectTransform *PlaceAtCenter(Transform *object, Transform *parent, const Vector2 &position, const Vector2 &size);

} // namespace ou
