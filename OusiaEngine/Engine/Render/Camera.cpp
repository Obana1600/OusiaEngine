#include "Camera.h"
#include "Math/Mathf.h"
#include "Platform/Screen.h"
#include "Scene/SceneManager.h"
#include "Scene/Transform.h"

using namespace ou;


void ou::Camera::Awake() {
	SceneManager::RegisterCamera(this);
}


void ou::Camera::OnDestroy() {
	SceneManager::UnregisterCamera(this);
}


Matrix4x4 ou::Camera::GetWorldToCameraMatrix() const {
	return GetTransform()->GetLocalToWorldMatrix().Inverse();
}


Matrix4x4 ou::Camera::GetProjectionMatrix() const {
	return Matrix4x4::Perspective(fieldOfView_ * Mathf::Deg2Rad, GetAspect(), nearClipPlane_, farClipPlane_);
}


Camera::PerCamera ou::Camera::MakePerCamera() const {
	PerCamera constants{};
	constants.view = GetWorldToCameraMatrix();
	constants.projection = GetProjectionMatrix();
	constants.viewProjection = constants.view * constants.projection;
	constants.worldPosition = GetTransform()->GetWorldPosition();

	return constants;
}


void ou::Camera::SetFieldOfView(float fieldOfView) {
	fieldOfView_ = fieldOfView;
}


float ou::Camera::GetFieldOfView() const {
	return fieldOfView_;
}


void ou::Camera::SetAspect(float aspect) {
	aspect_ = aspect;
}


float ou::Camera::GetAspect() const {
	if (aspect_ > 0.0f) { return aspect_; }

	const auto width = Screen::GetWidth();
	const auto height = Screen::GetHeight();
	if (width == 0 || height == 0) { return 1.0f; }
	return static_cast<float>(width) / static_cast<float>(height);
}


void ou::Camera::ResetAspect() {
	aspect_ = 0.0f;
}


void ou::Camera::SetNearClipPlane(float nearClipPlane) {
	nearClipPlane_ = nearClipPlane;
}


float ou::Camera::GetNearClipPlane() const {
	return nearClipPlane_;
}


void ou::Camera::SetFarClipPlane(float farClipPlane) {
	farClipPlane_ = farClipPlane;
}


float ou::Camera::GetFarClipPlane() const {
	return farClipPlane_;
}
