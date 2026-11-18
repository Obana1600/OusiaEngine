#include "SceneView.h"
#include "Editor/EditorFunc.h"
#include "Editor/Selection.h"
#include "Math/Mathf.h"
#include "Math/Matrix4x4.h"
#include "Scene/GameObject.h"
#include "Scene/Transform.h"

#include <algorithm>

#include <imgui/imgui.h>

using namespace ou;


namespace {

SceneView *sLastDrawn = nullptr;
int sLastDrawnFrame = -1;

constexpr float kFieldOfView = 60.0f;
constexpr float kNearClipPlane = 0.03f;
constexpr float kFarClipPlane = 10000.0f;

constexpr float kLookSensitivity = 0.2f;
constexpr float kZoomStep = 0.1f;
constexpr float kMinDistance = 0.01f;
constexpr float kFastMultiplier = 4.0f;
constexpr float kMinFlySpeed = 0.1f;
constexpr float kMaxFlySpeed = 100.0f;

float AxisValue(ImGuiKey positive, ImGuiKey negative) {
	return (ImGui::IsKeyDown(positive) ? 1.0f : 0.0f) - (ImGui::IsKeyDown(negative) ? 1.0f : 0.0f);
}

} // namespace



SceneView *ou::SceneView::FindVisible() {
	if (sLastDrawnFrame != ImGui::GetFrameCount()) { return nullptr; }
	return sLastDrawn;
}


ou::SceneView::SceneView() {
	SetTitle("Scene");
}


const SceneRenderTarget &ou::SceneView::GetRenderTarget() const {
	return renderTarget_;
}


Camera::PerCamera ou::SceneView::MakePerCamera() const {
	const auto rotation = MakeRotation();
	const auto position = MakePosition();
	const auto height = renderTarget_.GetHeight();
	const auto aspect = height > 0 ? static_cast<float>(renderTarget_.GetWidth()) / static_cast<float>(height) : 1.0f;

	Camera::PerCamera constants{};
	constants.view = Matrix4x4::SRT(Vector3::One, rotation, position).Inverse();
	constants.projection = Matrix4x4::Perspective(kFieldOfView * Mathf::Deg2Rad, aspect, kNearClipPlane, kFarClipPlane);
	constants.viewProjection = constants.view * constants.projection;
	constants.worldPosition = position;

	return constants;
}


void ou::SceneView::OnGui() {
	const auto available = ImGui::GetContentRegionAvail();
	if (available.x < 1.0f || available.y < 1.0f) { return; }

	// Game ビューと同じ比で描き、余りは帯にする（見える範囲を Game ビューとそろえる）
	const auto size = FitToPlayerAspect({available.x, available.y});
	renderTarget_.Resize(static_cast<uint32_t>(size.x), static_cast<uint32_t>(size.y));
	CenterNextItem({available.x, available.y}, size);
	ImGui::Image(ToTextureId(renderTarget_.GetColor()), {size.x, size.y});
	HandleInput(ImGui::IsItemHovered());

	sLastDrawn = this;
	sLastDrawnFrame = ImGui::GetFrameCount();
}


void ou::SceneView::HandleInput(bool isHovered) {
	const auto &io = ImGui::GetIO();
	const Vector2 mouseDelta{io.MouseDelta.x, io.MouseDelta.y};

	if (dragMode_ == DragMode::None && isHovered) {
		StartDrag();
	}

	switch (dragMode_) {
		case DragMode::Fly:
			if (!ImGui::IsMouseDown(ImGuiMouseButton_Right)) {
				dragMode_ = DragMode::None;
				break;
			}
			Fly(mouseDelta);
			break;

		case DragMode::Pan:
			if (!ImGui::IsMouseDown(ImGuiMouseButton_Middle)) {
				dragMode_ = DragMode::None;
				break;
			}
			Pan(mouseDelta);
			break;

		case DragMode::Orbit:
			if (!ImGui::IsMouseDown(ImGuiMouseButton_Left)) {
				dragMode_ = DragMode::None;
				break;
			}
			Orbit(mouseDelta);
			break;

		case DragMode::None:
			if (isHovered && io.MouseWheel != 0.0f) {
				Zoom(io.MouseWheel);
			}
			break;
	}

	if (ImGui::IsWindowFocused() && !io.WantTextInput && ImGui::IsKeyPressed(ImGuiKey_F, false)) {
		FrameSelected();
	}
}


void ou::SceneView::StartDrag() {
	if (ImGui::IsMouseClicked(ImGuiMouseButton_Right)) {
		dragMode_ = DragMode::Fly;
	} else if (ImGui::IsMouseClicked(ImGuiMouseButton_Middle)) {
		dragMode_ = DragMode::Pan;
	} else if (ImGui::GetIO().KeyAlt && ImGui::IsMouseClicked(ImGuiMouseButton_Left)) {
		dragMode_ = DragMode::Orbit;
	}

	if (dragMode_ != DragMode::None) {
		ImGui::SetWindowFocus();
	}
}


void ou::SceneView::Fly(const Vector2 &mouseDelta) {
	const auto &io = ImGui::GetIO();

	const auto position = MakePosition();
	yaw_ += mouseDelta.x * kLookSensitivity;
	pitch_ = Mathf::Clamp(pitch_ + mouseDelta.y * kLookSensitivity, -89.0f, 89.0f);
	const auto rotation = MakeRotation();
	pivot_ = position + rotation * Vector3::Forward * distance_;

	if (io.MouseWheel != 0.0f) {
		flySpeed_ = Mathf::Clamp(flySpeed_ * (1.0f + io.MouseWheel * 0.2f), kMinFlySpeed, kMaxFlySpeed);
	}

	const auto direction = rotation * Vector3::Forward * AxisValue(ImGuiKey_W, ImGuiKey_S) +
						   rotation * Vector3::Right * AxisValue(ImGuiKey_D, ImGuiKey_A) +
						   Vector3::Up * AxisValue(ImGuiKey_E, ImGuiKey_Q);
	const auto speed = flySpeed_ * (io.KeyShift ? kFastMultiplier : 1.0f);
	pivot_ += direction * (speed * io.DeltaTime);
}


void ou::SceneView::Pan(const Vector2 &mouseDelta) {
	const auto height = static_cast<float>(std::max(renderTarget_.GetHeight(), 1u));

	const auto metersPerPixel = 2.0f * distance_ * Mathf::Tan(kFieldOfView * 0.5f * Mathf::Deg2Rad) / height;

	const auto rotation = MakeRotation();
	pivot_ -= rotation * Vector3::Right * (mouseDelta.x * metersPerPixel);
	pivot_ += rotation * Vector3::Up * (mouseDelta.y * metersPerPixel);
}


void ou::SceneView::Orbit(const Vector2 &mouseDelta) {
	yaw_ += mouseDelta.x * kLookSensitivity;
	pitch_ = Mathf::Clamp(pitch_ + mouseDelta.y * kLookSensitivity, -89.0f, 89.0f);
}


void ou::SceneView::Zoom(float wheel) {
	distance_ = std::max(distance_ * (1.0f - wheel * kZoomStep), kMinDistance);
}


void ou::SceneView::FrameSelected() {
	if (const auto *gameObject = Selection::GetActiveGameObject()) {
		pivot_ = gameObject->GetTransform()->GetWorldPosition();
	}
}


Quaternion ou::SceneView::MakeRotation() const {
	return Quaternion::Euler(pitch_ * Mathf::Deg2Rad, yaw_ * Mathf::Deg2Rad, 0.0f);
}


Vector3 ou::SceneView::MakePosition() const {
	return pivot_ - MakeRotation() * Vector3::Forward * distance_;
}
