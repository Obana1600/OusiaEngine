#include "Camera.h"
#include "Scene/GameObject.h"
#include "Scene/Scene.h"
#include "Scene/Transform.h"

using namespace ou;


void ou::Camera::Awake() {
	GetGameObject()->GetScene()->RegisterCamera(this);
}


void ou::Camera::OnDestroy() {
	GetGameObject()->GetScene()->UnregisterCamera(this);
}


Matrix4x4 ou::Camera::GetViewMatrix() const {
	return GetTransform()->GetLocalToWorldMatrix().Inverse();
}


Matrix4x4 ou::Camera::GetProjectionMatrix() const {
	return Matrix4x4::Perspective(fovY_, aspect_, nearZ_, farZ_);
}


Camera::PerCamera ou::Camera::MakePerCamera() const {
	PerCamera constants{};
	constants.view = GetViewMatrix();
	constants.projection = GetProjectionMatrix();
	constants.viewProjection = constants.view * constants.projection;
	constants.worldPosition = GetTransform()->GetWorldPosition();

	return constants;
}


void ou::Camera::SetFovY(float fovY) {
	fovY_ = fovY;
}


float ou::Camera::GetFovY() const {
	return fovY_;
}


void ou::Camera::SetAspect(float aspect) {
	aspect_ = aspect;
}


float ou::Camera::GetAspect() const {
	return aspect_;
}


void ou::Camera::SetClipPlanes(float nearZ, float farZ) {
	nearZ_ = nearZ;
	farZ_ = farZ;
}


float ou::Camera::GetNearZ() const {
	return nearZ_;
}


float ou::Camera::GetFarZ() const {
	return farZ_;
}
