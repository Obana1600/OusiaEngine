#pragma once

#include "Animation/Tween/TweenHandle.h"
#include "Math/Quaternion.h"
#include "Math/Vector3.h"


namespace ou {

// 前方宣言
class Transform;

TweenHandle DoMove(Transform *transform, const Vector3 &to, float duration);
TweenHandle DoLocalMove(Transform *transform, const Vector3 &to, float duration);

TweenHandle DoRotate(Transform *transform, const Quaternion &to, float duration);
TweenHandle DoLocalRotate(Transform *transform, const Quaternion &to, float duration);

TweenHandle DoScale(Transform *transform, const Vector3 &to, float duration);

} // namespace ou
