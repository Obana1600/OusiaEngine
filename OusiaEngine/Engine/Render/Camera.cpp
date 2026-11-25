#include "Camera.h"
#include "Math/Mathf.h"
#include "Platform/Screen.h"
#include "Scene/SceneManager.h"
#include "Scene/Transform.h"
#include "Serialize/Serializer.h"
#include "Render/Gizmos.h"

using namespace ou;


void ou::Camera::Awake() {
	SceneManager::RegisterCamera(this);
}


void ou::Camera::OnDestroy() {
	SceneManager::UnregisterCamera(this);
}


void ou::Camera::Serialize(Serializer &serializer) {
	Behavior::Serialize(serializer);

	serializer.Field("fieldOfView", fieldOfView_);
	serializer.Field("aspect", aspect_);
	serializer.Field("nearClipPlane", nearClipPlane_);
	serializer.Field("farClipPlane", farClipPlane_);
	serializer.Field("renderPostProcessing", renderPostProcessing_);
	serializer.Field("volumeLayerMask", volumeLayerMask_.value);
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


void ou::Camera::SetRenderPostProcessing(bool renderPostProcessing) {
	renderPostProcessing_ = renderPostProcessing;
}


bool ou::Camera::GetRenderPostProcessing() const {
	return renderPostProcessing_;
}


void ou::Camera::SetVolumeLayerMask(LayerMask volumeLayerMask) {
	volumeLayerMask_ = volumeLayerMask;
}


LayerMask ou::Camera::GetVolumeLayerMask() const {
	return volumeLayerMask_;
}


void ou::Camera::OnDrawGizmos() {
	Gizmos::DrawIcon(GetTransform()->GetWorldPosition(), "Camera");
}


void ou::Camera::OnDrawGizmosSelected() {
	const auto *transform = GetTransform();
	Gizmos::SetColor(Color(0.9f, 0.9f, 0.9f, 0.6f));
	Gizmos::SetMatrix(Matrix4x4::SRT(Vector3::One, transform->GetWorldRotation(), transform->GetWorldPosition()));
	Gizmos::DrawFrustum(Vector3::Zero, fieldOfView_, farClipPlane_, nearClipPlane_, GetAspect());
}
