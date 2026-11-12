#include "TweenShortcuts.h"
#include "Animation/Tween/TweenManager.h"
#include "Scene/Transform.h"

using namespace ou;


TweenHandle ou::DoMove(Transform *transform, const Vector3 &to, float duration) {
	return TweenManager::To<Vector3>(
		transform,
		[transform] { return transform->GetWorldPosition(); },
		[transform](const Vector3 &value) { transform->SetWorldPosition(value); },
		to,
		duration);
}


TweenHandle ou::DoLocalMove(Transform *transform, const Vector3 &to, float duration) {
	return TweenManager::To<Vector3>(
		transform,
		[transform] { return transform->GetLocalPosition(); },
		[transform](const Vector3 &value) { transform->SetLocalPosition(value); },
		to,
		duration);
}


TweenHandle ou::DoRotate(Transform *transform, const Quaternion &to, float duration) {
	return TweenManager::To<Quaternion>(
		transform,
		[transform] { return transform->GetWorldRotation(); },
		[transform](const Quaternion &value) { transform->SetWorldRotation(value); },
		to,
		duration);
}


TweenHandle ou::DoLocalRotate(Transform *transform, const Quaternion &to, float duration) {
	return TweenManager::To<Quaternion>(
		transform,
		[transform] { return transform->GetLocalRotation(); },
		[transform](const Quaternion &value) { transform->SetLocalRotation(value); },
		to,
		duration);
}


TweenHandle ou::DoScale(Transform *transform, const Vector3 &to, float duration) {
	return TweenManager::To<Vector3>(
		transform,
		[transform] { return transform->GetLocalScale(); },
		[transform](const Vector3 &value) { transform->SetLocalScale(value); },
		to,
		duration);
}
