#include "SceneView.h"
#include "Asset/AssetDatabase.h"
#include "Editor/EditorFunc.h"
#include "Editor/EditorPayload.h"
#include "Editor/EditorSceneManager.h"
#include "Editor/PrefabUtility.h"
#include "Editor/Selection.h"
#include "Editor/Tools.h"
#include "Math/Mathf.h"
#include "Math/Matrix4x4.h"
#include "Render/Asset/Model.h"
#include "Scene/Component.h"
#include "Scene/GameObject.h"
#include "Scene/SceneManager.h"
#include "Scene/Transform.h"

#include <algorithm>
#include <cmath>
#include <utility>

// clang-format off
#include <imgui/imgui.h>
#include <ImGuizmo/ImGuizmo.h> // imgui/imgui.h の後ろに置く
// clang-format on

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

constexpr std::pair<Tool, const char *> kToolButtons[] = {
	{Tool::View, "View (Q)"},
	{Tool::Move, "Move (W)"},
	{Tool::Rotate, "Rotate (E)"},
	{Tool::Scale, "Scale (R)"},
};

float AxisValue(ImGuiKey positive, ImGuiKey negative) {
	return (ImGui::IsKeyDown(positive) ? 1.0f : 0.0f) - (ImGui::IsKeyDown(negative) ? 1.0f : 0.0f);
}

ImGuizmo::OPERATION ToOperation(Tool tool) {
	switch (tool) {
		default:
		case Tool::Move:
			return ImGuizmo::TRANSLATE;
		case Tool::Rotate:
			return ImGuizmo::ROTATE;
		case Tool::Scale:
			return ImGuizmo::SCALE;
	}
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
	DrawToolbar();

	const auto available = ImGui::GetContentRegionAvail();
	if (available.x < 1.0f || available.y < 1.0f) { return; }

	// Game ビューと同じ比で描き、余りは帯にする（見える範囲を Game ビューとそろえる）
	const auto size = FitToPlayerAspect({available.x, available.y});
	renderTarget_.Resize(static_cast<uint32_t>(size.x), static_cast<uint32_t>(size.y));
	CenterNextItem({available.x, available.y}, size);
	ImGui::Image(ToTextureId(renderTarget_.GetColor()), {size.x, size.y});
	const auto isImageHovered = ImGui::IsItemHovered();
	const auto imageMin = ImGui::GetItemRectMin();
	AcceptAssetDrop();
	const auto isHandleHot = DrawTransformHandle();
	// パネルは子ウィンドウなので、上に乗っているあいだは絵の IsItemHovered が false になり、カメラは動かない
	DrawSceneGuis(Rect(imageMin.x, imageMin.y, size.x, size.y));
	HandleInput(isImageHovered && !isHandleHot);

	sLastDrawn = this;
	sLastDrawnFrame = ImGui::GetFrameCount();
}


void ou::SceneView::DrawToolbar() {
	for (const auto &[tool, label] : kToolButtons) {
		if (ImGui::RadioButton(label, Tools::GetCurrent() == tool)) { Tools::SetCurrent(tool); }
		ImGui::SameLine();
	}

	const auto isLocal = Tools::GetPivotRotation() == PivotRotation::Local;
	if (ImGui::Button(isLocal ? "Local" : "Global")) {
		Tools::SetPivotRotation(isLocal ? PivotRotation::Global : PivotRotation::Local);
	}
}


void ou::SceneView::SelectToolByKey() {
	if (ImGui::IsKeyPressed(ImGuiKey_Q, false)) { Tools::SetCurrent(Tool::View); }
	if (ImGui::IsKeyPressed(ImGuiKey_W, false)) { Tools::SetCurrent(Tool::Move); }
	if (ImGui::IsKeyPressed(ImGuiKey_E, false)) { Tools::SetCurrent(Tool::Rotate); }
	if (ImGui::IsKeyPressed(ImGuiKey_R, false)) { Tools::SetCurrent(Tool::Scale); }
}


bool ou::SceneView::DrawTransformHandle() {
	auto *gameObject = Selection::GetActiveGameObject();
	const auto tool = Tools::GetCurrent();
	if (!gameObject || tool == Tool::View || tool == Tool::None) { return false; }

	const auto min = ImGui::GetItemRectMin();
	const auto size = ImGui::GetItemRectSize();
	ImGuizmo::SetDrawlist();
	ImGuizmo::SetRect(min.x, min.y, size.x, size.y);
	ImGuizmo::SetOrthographic(false);
	ImGuizmo::Enable(dragMode_ == DragMode::None);

	const auto camera = MakePerCamera();
	auto *transform = gameObject->GetTransform();
	auto world = transform->GetLocalToWorldMatrix();
	const auto mode = (tool == Tool::Scale || Tools::GetPivotRotation() == PivotRotation::Local) ? ImGuizmo::LOCAL : ImGuizmo::WORLD;

	if (ImGuizmo::Manipulate(&camera.view.m11, &camera.projection.m11, ToOperation(tool), mode, &world.m11)) {
		const auto *parent = transform->GetParent();
		const auto local = parent ? world * parent->GetLocalToWorldMatrix().Inverse() : world;

		switch (tool) {
			case Tool::Move:
				transform->SetLocalPosition(local.GetPosition());
				break;
			case Tool::Rotate:
				transform->SetLocalRotation(local.GetRotation());
				break;
			case Tool::Scale:
				transform->SetLocalScale(local.GetLossyScale());
				break;
			default:
				break;
		}
		EditorSceneManager::MarkSceneDirty(gameObject->GetScene());
	}

	return ImGuizmo::IsOver() || ImGuizmo::IsUsing();
}


void ou::SceneView::DrawSceneGuis(const Rect &sceneRect) {
	auto *gameObject = Selection::GetActiveGameObject();
	if (!gameObject) {
		sceneGuiTargetId_ = 0;
		sceneGuiComponentIds_.clear();
		sceneGuiEditors_.clear();
		return;
	}

	// Inspector と同じく、選択かコンポーネントが変わったときだけ Editor を作り直す
	std::vector<Component *> components;
	std::vector<uint32_t> componentIds;
	for (auto *component : gameObject->GetComponents<Component>()) {
		if (component->IsDestroyed()) { continue; }
		components.push_back(component);
		componentIds.push_back(component->GetInstanceId());
	}
	if (gameObject->GetInstanceId() != sceneGuiTargetId_ || componentIds != sceneGuiComponentIds_) {
		sceneGuiTargetId_ = gameObject->GetInstanceId();
		sceneGuiComponentIds_ = std::move(componentIds);
		sceneGuiEditors_.clear();
		for (auto *component : components) {
			sceneGuiEditors_.push_back(Editor::Create(component));
		}
	}

	for (size_t i = 0; i < sceneGuiEditors_.size(); i++) {
		ImGui::PushID(static_cast<int>(i));
		sceneGuiEditors_[i]->OnSceneGui(sceneRect);
		ImGui::PopID();
	}
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

	if (ImGui::IsWindowFocused() && !io.WantTextInput) {
		if (ImGui::IsKeyPressed(ImGuiKey_F, false)) { FrameSelected(); }
		if (dragMode_ == DragMode::None) { SelectToolByKey(); }
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


void ou::SceneView::AcceptAssetDrop() {
	if (!ImGui::BeginDragDropTarget()) { return; }

	const auto *payload = ImGui::AcceptDragDropPayload(EditorPayload::kAsset);
	ImGui::EndDragDropTarget();
	if (!payload) { return; }

	auto *scene = SceneManager::GetActiveScene();
	const auto asset = EditorPayload::DecodeAsset(payload->Data, payload->DataSize);
	if (!scene || !asset.subAsset.empty()) { return; }

	// 直前の項目は Scene の画像なので、その矩形とマウスの位置から、マウスの先を求める
	const auto min = ImGui::GetItemRectMin();
	const auto itemSize = ImGui::GetItemRectSize();

	GameObject *root = nullptr;
	if (const auto model = AssetDatabase::LoadAssetAtPath<Model>(asset.path)) {
		root = model->Instantiate(scene);
	} else if (const auto prefab = AssetDatabase::LoadAssetAtPath<GameObject>(asset.path)) {
		root = PrefabUtility::InstantiatePrefab(prefab.get(), scene);
	}
	if (!root) { return; }

	// マウスの先の、高さ 0 の面に置く（面に当たらなければ、注視点と同じ距離の先）
	// Unity はコライダーに当てるが、ここでは地面の面だけを見る
	const auto mouse = ImGui::GetMousePos();
	const auto x = (mouse.x - min.x) / std::max(itemSize.x, 1.0f) * 2.0f - 1.0f;
	const auto y = 1.0f - (mouse.y - min.y) / std::max(itemSize.y, 1.0f) * 2.0f;
	const auto tanHalf = std::tan(kFieldOfView * 0.5f * Mathf::Deg2Rad);
	const auto aspect = itemSize.x / std::max(itemSize.y, 1.0f);
	const auto direction = (MakeRotation() * Vector3{x * tanHalf * aspect, y * tanHalf, 1.0f}).Normalized();
	const auto origin = MakePosition();

	auto position = origin + direction * distance_;
	if (direction.y < -0.0001f && origin.y > 0.0f) {
		position = origin + direction * (-origin.y / direction.y);
	}
	root->GetTransform()->SetWorldPosition(position);

	Selection::SetActiveGameObject(root);
	EditorSceneManager::MarkSceneDirty(scene);
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
