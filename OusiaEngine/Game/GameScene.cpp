#include "GameScene.h"
#include "Core/Time.h"
#include "Diagnostics/Log.h"
#include "Input/Input.h"
#include "Math/Mathf.h"
#include "Math/Quaternion.h"
#include "Physics/BoxCollider.h"
#include "Physics/Collision.h"
#include "Physics/Physics.h"
#include "Physics/Rigidbody.h"
#include "Physics/SphereCollider.h"
#include "Platform/Screen.h"
#include "Render/Asset/Material.h"
#include "Render/Asset/Mesh.h"
#include "Render/Camera.h"
#include "Render/DirectionalLight.h"
#include "Render/MeshRenderer.h"
#include "Scene/GameObject.h"

#ifdef USE_IMGUI
#include <imgui/imgui.h>
#include <imgui/imgui_stdlib.h>
#endif

#include <cmath>
#include <cstdint>
#include <format>
#include <iterator>
#include <utility>

using namespace ou;


namespace {

constexpr size_t kMaxEventLogLines = 200;
constexpr size_t kMaxProjectiles = 100;

// ContactReporter が書き、PhysicsEditor が出す
std::vector<std::string> sEventLog;

void AddEventLog(std::string text) {
	Log::Info("EVENT {}", text);
	sEventLog.push_back(std::move(text));
	if (sEventLog.size() > kMaxEventLogLines) { sEventLog.erase(sEventLog.begin()); }
}

const char *const kPresetNames[] = {"いろいろ", "ピラミッド", "ドミノ", "床だけ"};
const char *const kShapeNames[] = {"Box", "Sphere"};
const char *const kBodyTypeNames[] = {"Static（Rigidbody なし）", "Dynamic", "Kinematic"};
const char *const kFreezeNames[] = {"位置 X", "位置 Y", "位置 Z", "回転 X", "回転 Y", "回転 Z"};

// Unity の Sphere と同じ半径 0.5 の球
std::shared_ptr<Mesh> CreateSphereMesh(int slices, int stacks) {
	std::vector<Mesh::Vertex> vertices;
	for (int i = 0; i <= stacks; i++) {
		const auto theta = Mathf::PI * static_cast<float>(i) / static_cast<float>(stacks);
		for (int j = 0; j <= slices; j++) {
			const auto phi = 2.0f * Mathf::PI * static_cast<float>(j) / static_cast<float>(slices);
			const Vector3 normal = {std::sin(theta) * std::cos(phi), std::cos(theta), std::sin(theta) * std::sin(phi)};
			vertices.push_back({normal * 0.5f, normal,
								{static_cast<float>(j) / static_cast<float>(slices), static_cast<float>(i) / static_cast<float>(stacks)}});
		}
	}

	// Mesh::CreateCube と同じく、外から見て時計回り
	std::vector<uint32_t> indices;
	for (int i = 0; i < stacks; i++) {
		for (int j = 0; j < slices; j++) {
			const auto a = static_cast<uint32_t>(i * (slices + 1) + j);
			const auto b = a + static_cast<uint32_t>(slices + 1);
			indices.insert(indices.end(), {a, a + 1, b + 1, a, b + 1, b});
		}
	}

	auto mesh = std::make_shared<Mesh>();
	mesh->SetName("Sphere");
	mesh->SetVertices(std::move(vertices));
	mesh->SetIndices(std::move(indices));
	mesh->Upload();
	return mesh;
}

Color LerpColor(const Color &a, const Color &b, float t) {
	return {Mathf::Lerp(a.r, b.r, t), Mathf::Lerp(a.g, b.g, t), Mathf::Lerp(a.b, b.b, t), Mathf::Lerp(a.a, b.a, t)};
}

PhysicsEditor::BodySpec MakeFloor() {
	PhysicsEditor::BodySpec spec;
	spec.name = "Floor";
	spec.bodyType = PhysicsEditor::BodyType::Static;
	spec.position = {0.0f, -0.5f, 0.0f};
	spec.scale = {40.0f, 1.0f, 40.0f};
	spec.color = {0.45f, 0.47f, 0.5f, 1.0f};
	return spec;
}

PhysicsEditor::BodySpec MakeBox(std::string name, const Vector3 &position, const Vector3 &scale, const Color &color) {
	PhysicsEditor::BodySpec spec;
	spec.name = std::move(name);
	spec.position = position;
	spec.scale = scale;
	spec.color = color;
	return spec;
}

PhysicsEditor::BodySpec MakeSphere(std::string name, const Vector3 &position, float diameter, const Color &color) {
	auto spec = MakeBox(std::move(name), position, Vector3::One * diameter, color);
	spec.shape = PhysicsEditor::ShapeType::Sphere;
	return spec;
}

std::string FormatVector(const Vector3 &v) {
	return std::format("({:.2f}, {:.2f}, {:.2f})", v.x, v.y, v.z);
}

} // namespace



void ou::BuildGameScene(Scene &scene) {
	auto *light = scene.CreateGameObject("Directional Light");
	light->AddComponent<DirectionalLight>();
	light->GetTransform()->SetLocalEulerAngles(Vector3{50.0f, -30.0f, 0.0f} * Mathf::Deg2Rad);

	scene.CreateGameObject("Physics Editor")->AddComponent<PhysicsEditor>();
}


// ----- PhysicsEditor -----

void ou::PhysicsEditor::Start() {
	auto *cameraObject = GetGameObject()->GetScene()->CreateGameObject("Main Camera");
	camera_ = cameraObject->AddComponent<Camera>();

	cubeMesh_ = Mesh::CreateCube();
	sphereMesh_ = CreateSphereMesh(32, 16);

	LoadPreset(0);
	UpdateCamera();

#ifndef USE_IMGUI
	// Release は操作の画面がないので、すぐ動かす
	Play();
#endif
}


void ou::PhysicsEditor::Update() {
	UpdateCamera();
	HandleMouseAndKeys();

#ifdef USE_IMGUI
	ShowToolbar();
	ShowHierarchy();
	ShowInspector();
	ShowSettings();
	ShowEventLog();
#endif

	if (isDirty_ && !isPlaying_) {
		isDirty_ = false;
		Rebuild();
	}

	UpdateHighlight();
	UpdateTimeScale();
}


void ou::PhysicsEditor::LoadPreset(int index) {
	presetIndex_ = index;
	specs_.clear();
	specs_.push_back(MakeFloor());

	switch (index) {
		case 0: {
			// 斜面を転がる球・積んだ箱に落ちる重い箱・トリガー・往復する Kinematic
			auto ramp = MakeBox("Ramp", {-7.0f, 2.0f, 0.0f}, {9.0f, 0.4f, 4.0f}, {0.55f, 0.5f, 0.45f, 1.0f});
			ramp.bodyType = BodyType::Static;
			ramp.rotation = {0.0f, 0.0f, -25.0f};
			specs_.push_back(ramp);
			for (int i = 0; i < 3; i++) {
				specs_.push_back(MakeSphere(std::format("Ball {}", i), {-10.0f, 6.0f + static_cast<float>(i) * 1.5f, -1.0f + static_cast<float>(i)}, 0.8f,
											{0.9f, 0.45f, 0.2f, 1.0f}));
			}
			for (int i = 0; i < 5; i++) {
				specs_.push_back(MakeBox(std::format("Stack {}", i), {4.0f, 0.5f + static_cast<float>(i), 0.0f}, Vector3::One, {0.3f, 0.6f, 0.9f, 1.0f}));
			}
			auto heavy = MakeBox("Heavy", {4.2f, 12.0f, 0.0f}, {1.5f, 1.5f, 1.5f}, {0.8f, 0.2f, 0.25f, 1.0f});
			heavy.mass = 20.0f;
			specs_.push_back(heavy);

			auto trigger = MakeBox("Trigger Zone", {0.0f, 1.0f, -6.0f}, {4.0f, 2.0f, 4.0f}, {0.3f, 0.85f, 0.4f, 1.0f});
			trigger.bodyType = BodyType::Static;
			trigger.isTrigger = true;
			specs_.push_back(trigger);
			auto visitor = MakeSphere("Visitor", {0.0f, 8.0f, -6.0f}, 1.0f, {0.95f, 0.85f, 0.3f, 1.0f});
			specs_.push_back(visitor);

			auto paddle = MakeBox("Paddle", {0.0f, 0.5f, 6.0f}, {1.0f, 1.0f, 3.0f}, {0.6f, 0.4f, 0.8f, 1.0f});
			paddle.bodyType = BodyType::Kinematic;
			paddle.isMoving = true;
			specs_.push_back(paddle);
			for (int i = 0; i < 4; i++) {
				specs_.push_back(MakeBox(std::format("Pushed {}", i), {-3.0f + static_cast<float>(i) * 2.0f, 0.4f, 6.0f}, {0.8f, 0.8f, 0.8f},
										 {0.85f, 0.85f, 0.85f, 1.0f}));
			}
			break;
		}

		case 1: {
			// 箱のピラミッド
			constexpr int kLayers = 7;
			for (int layer = 0; layer < kLayers; layer++) {
				for (int i = 0; i < kLayers - layer; i++) {
					const auto x = (static_cast<float>(i) - static_cast<float>(kLayers - layer - 1) * 0.5f) * 1.02f;
					const auto hue = static_cast<float>(layer) / static_cast<float>(kLayers);
					specs_.push_back(MakeBox(std::format("Box {}-{}", layer, i), {x, 0.5f + static_cast<float>(layer), 0.0f}, Vector3::One,
											 {0.3f + hue * 0.6f, 0.5f, 0.9f - hue * 0.6f, 1.0f}));
				}
			}
			break;
		}

		case 2: {
			// ドミノ: 最初の 1 枚を傾けておく
			for (int i = 0; i < 16; i++) {
				auto domino = MakeBox(std::format("Domino {}", i), {-12.0f + static_cast<float>(i) * 1.2f, 1.0f, 0.0f}, {0.25f, 2.0f, 1.2f},
									  {0.95f, 0.95f - static_cast<float>(i) * 0.04f, 0.5f, 1.0f});
				if (i == 0) {
					domino.rotation = {0.0f, 0.0f, -20.0f};
					domino.position.y = 1.05f;
				}
				specs_.push_back(domino);
			}
			break;
		}

		default:
			break;
	}

	selected_ = -1;
	Rebuild();
}


void ou::PhysicsEditor::Rebuild() {
	// GameObject はフレームの終わりに消える。編集中は物理が止まっているので、新しいものと重なっても困らない
	for (auto *object : objects_) {
		Object::Destroy(object);
	}
	objects_.clear();

	for (const auto &spec : specs_) {
		objects_.push_back(Spawn(spec));
	}
}


GameObject *ou::PhysicsEditor::Spawn(const BodySpec &spec) {
	auto *object = GetGameObject()->GetScene()->CreateGameObject(spec.name);
	auto *transform = object->GetTransform();
	transform->SetLocalPosition(spec.position);
	transform->SetLocalEulerAngles(spec.rotation * Mathf::Deg2Rad);
	// 球はコライダーが一番大きい軸を使うので、見た目もそろえる
	transform->SetLocalScale(spec.shape == ShapeType::Sphere ? Vector3::One * spec.scale.x : spec.scale);

	auto *renderer = object->AddComponent<MeshRenderer>();
	renderer->SetMesh(spec.shape == ShapeType::Sphere ? sphereMesh_ : cubeMesh_);
	auto material = Material::Create();
	material->SetColor(spec.color);
	renderer->SetSharedMaterial(material);

	// Unity の Cube（大きさ 1）・Sphere（半径 0.5）と同じ大きさなので、Transform のスケールで大きさを決める
	Collider *collider = nullptr;
	if (spec.shape == ShapeType::Sphere) {
		collider = object->AddComponent<SphereCollider>();
	} else {
		collider = object->AddComponent<BoxCollider>();
	}
	collider->SetIsTrigger(spec.isTrigger);

	if (spec.bodyType != BodyType::Static) {
		auto *rigidbody = object->AddComponent<Rigidbody>();
		rigidbody->SetMass(spec.mass);
		rigidbody->SetUseGravity(spec.useGravity);
		rigidbody->SetLinearDamping(spec.linearDamping);
		rigidbody->SetAngularDamping(spec.angularDamping);
		rigidbody->SetIsKinematic(spec.bodyType == BodyType::Kinematic);

		// RigidbodyConstraints は FreezePositionX が 1 << 1 から並ぶ
		uint8_t constraints = 0;
		for (int i = 0; i < 6; i++) {
			if (spec.freeze[i]) { constraints |= static_cast<uint8_t>(1 << (i + 1)); }
		}
		rigidbody->SetConstraints(static_cast<RigidbodyConstraints>(constraints));

		if (spec.bodyType == BodyType::Kinematic && spec.isMoving) {
			object->AddComponent<KinematicMover>();
		}
	}

	object->AddComponent<ContactReporter>();
	return object;
}


void ou::PhysicsEditor::Play() {
	if (isPlaying_) { return; }

	// 置いたときの状態から始める
	Rebuild();
	isDirty_ = false;
	isPlaying_ = true;
	isPaused_ = false;

	for (size_t i = 0; i < specs_.size(); i++) {
		if (auto *rigidbody = objects_[i]->GetComponent<Rigidbody>()) {
			rigidbody->SetLinearVelocity(specs_[i].initialVelocity);
		}
	}
	AddEventLog("----- Play -----");
}


void ou::PhysicsEditor::Stop() {
	if (!isPlaying_) { return; }

	isPlaying_ = false;
	isPaused_ = false;
	for (auto *projectile : projectiles_) {
		Object::Destroy(projectile);
	}
	projectiles_.clear();

	// 動いた物体を捨てて、仕様から作り直す
	Rebuild();
	AddEventLog("----- Stop -----");
}


void ou::PhysicsEditor::UpdateTimeScale() {
	// timeScale が 0 の間は FixedUpdate も物理の 1 歩も回らない
	if (!isPlaying_ || (isPaused_ && !isStepRequested_)) {
		Time::SetTimeScale(0.0f);
		return;
	}

	if (isStepRequested_) {
		// 次のフレームの経過時間がこのフレームと同じくらいなら、ちょうど 1 歩ぶん進む
		isStepRequested_ = false;
		Time::SetTimeScale(Time::GetFixedDeltaTime() / Mathf::Max(Time::GetUnscaledDeltaTime(), 0.001f));
		return;
	}

	Time::SetTimeScale(playSpeed_);
}


void ou::PhysicsEditor::UpdateCamera() {
	const auto rotation = Quaternion::Euler(Vector3{cameraPitch_, cameraYaw_, 0.0f} * Mathf::Deg2Rad);
	auto *transform = camera_->GetTransform();
	transform->SetLocalRotation(rotation);
	transform->SetLocalPosition(cameraTarget_ - rotation * Vector3::Forward * cameraDistance_);
}


void ou::PhysicsEditor::HandleMouseAndKeys() {
#ifdef USE_IMGUI
	const auto &io = ImGui::GetIO();
	const auto isMouseFree = !io.WantCaptureMouse;
	const auto isKeyboardFree = !io.WantCaptureKeyboard;
#else
	const auto isMouseFree = true;
	const auto isKeyboardFree = true;
#endif

	const auto delta = Input::GetMouseDelta();
	if (isMouseFree) {
		// 右ドラッグ: 回す
		if (Input::GetMouseButton(1)) {
			cameraYaw_ += delta.x * 0.3f;
			cameraPitch_ = Mathf::Clamp(cameraPitch_ + delta.y * 0.3f, -89.0f, 89.0f);
		}

		// 中ドラッグ: 注視点を動かす
		if (Input::GetMouseButton(2)) {
			const auto rotation = camera_->GetTransform()->GetWorldRotation();
			const auto speed = cameraDistance_ * 0.0015f;
			cameraTarget_ += (rotation * Vector3::Left) * (delta.x * speed) + (rotation * Vector3::Up) * (delta.y * speed);
		}

		// ホイール: 近づく・離れる
		const auto scroll = Input::GetMouseScrollDelta();
		if (scroll.y != 0.0f) {
			cameraDistance_ = Mathf::Clamp(cameraDistance_ * (1.0f - scroll.y * 0.1f), 2.0f, 200.0f);
		}

		// 左クリック: Raycast で選ぶ
		if (Input::GetMouseButtonDown(0)) {
			SelectByRaycast(Input::GetMousePosition());
		}
	}

	if (!isKeyboardFree) { return; }

	// Space（Play 中）: マウスの向きに球を撃つ
	if (isPlaying_ && Input::GetKeyDown(KeyCode::Space)) {
		Vector3 origin;
		Vector3 direction;
		if (ScreenPointToRay(Input::GetMousePosition(), &origin, &direction)) {
			ShootBall(origin + direction, direction);
		}
	}

	if (Input::GetKeyDown(KeyCode::Return)) {
		isPlaying_ ? Stop() : Play();
	}
	if (Input::GetKeyDown(KeyCode::Delete) && !isPlaying_ && selected_ >= 0) {
		specs_.erase(specs_.begin() + selected_);
		selected_ = -1;
		isDirty_ = true;
	}
}


void ou::PhysicsEditor::ShootBall(const Vector3 &origin, const Vector3 &direction) {
	auto spec = MakeSphere("Projectile", origin, 0.6f, {1.0f, 0.55f, 0.15f, 1.0f});
	spec.mass = 2.0f;

	auto *object = Spawn(spec);
	object->GetComponent<Rigidbody>()->SetLinearVelocity(direction * shootSpeed_);
	projectiles_.push_back(object);

	if (projectiles_.size() > kMaxProjectiles) {
		Object::Destroy(projectiles_.front());
		projectiles_.erase(projectiles_.begin());
	}
}


bool ou::PhysicsEditor::ScreenPointToRay(const Vector2 &screenPoint, Vector3 *origin, Vector3 *direction) const {
	const auto width = static_cast<float>(Screen::GetWidth());
	const auto height = static_cast<float>(Screen::GetHeight());
	if (width <= 0.0f || height <= 0.0f) { return false; }

	// 画面の点を、カメラの近い面（z = 0）と遠い面（z = 1）の点に戻す
	const auto x = screenPoint.x / width * 2.0f - 1.0f;
	const auto y = 1.0f - screenPoint.y / height * 2.0f;
	const auto inverse = camera_->MakePerCamera().viewProjection.Inverse();
	const auto nearPoint = inverse.MultiplyPoint({x, y, 0.0f});
	const auto farPoint = inverse.MultiplyPoint({x, y, 1.0f});

	*origin = nearPoint;
	*direction = (farPoint - nearPoint).Normalize();
	return true;
}


void ou::PhysicsEditor::SelectByRaycast(const Vector2 &screenPoint) {
	Vector3 origin;
	Vector3 direction;
	if (!ScreenPointToRay(screenPoint, &origin, &direction)) { return; }

	// 編集中は物理が止まっていて形がまだないことがあるので、先に Transform を渡す
	Physics::SyncTransform();

	RaycastHit hit;
	selected_ = -1;
	if (!Physics::Raycast(origin, direction, &hit, Mathf::Infinity, QueryTriggerInteracation::Collide)) { return; }

	const auto *hitObject = hit.collider->GetGameObject();
	for (size_t i = 0; i < objects_.size(); i++) {
		if (objects_[i] == hitObject) {
			selected_ = static_cast<int>(i);
			return;
		}
	}
}


void ou::PhysicsEditor::UpdateHighlight() {
	// 選んでいる物体は白に寄せて明るくする
	for (size_t i = 0; i < objects_.size() && i < specs_.size(); i++) {
		const auto &material = objects_[i]->GetComponent<MeshRenderer>()->GetSharedMaterial();
		const auto isSelected = (static_cast<int>(i) == selected_);
		material->SetColor(isSelected ? LerpColor(specs_[i].color, Color::White, 0.5f) : specs_[i].color);
	}
}


#ifdef USE_IMGUI

void ou::PhysicsEditor::ShowToolbar() {
	ImGui::SetNextWindowPos({10.0f, 10.0f}, ImGuiCond_FirstUseEver);
	ImGui::SetNextWindowSize({640.0f, 110.0f}, ImGuiCond_FirstUseEver);
	if (!ImGui::Begin("Physics Editor")) {
		ImGui::End();
		return;
	}

	if (!isPlaying_) {
		if (ImGui::Button("▶ Play")) { Play(); }
	} else if (ImGui::Button("■ Stop")) {
		Stop();
	}
	ImGui::SameLine();
	ImGui::BeginDisabled(!isPlaying_);
	if (ImGui::Button(isPaused_ ? "再開" : "一時停止")) { isPaused_ = !isPaused_; }
	ImGui::SameLine();
	ImGui::BeginDisabled(!isPaused_);
	if (ImGui::Button("1 歩")) { isStepRequested_ = true; }
	ImGui::EndDisabled();
	ImGui::EndDisabled();
	ImGui::SameLine();
	ImGui::SetNextItemWidth(140.0f);
	ImGui::SliderFloat("速さ", &playSpeed_, 0.05f, 2.0f, "%.2f 倍");

	ImGui::BeginDisabled(isPlaying_);
	ImGui::SetNextItemWidth(160.0f);
	if (ImGui::Combo("##preset", &presetIndex_, kPresetNames, static_cast<int>(std::size(kPresetNames)))) {
		LoadPreset(presetIndex_);
	}
	ImGui::SameLine();
	if (ImGui::Button("配置を読み直す")) { LoadPreset(presetIndex_); }
	ImGui::EndDisabled();
	ImGui::SameLine();
	ImGui::Text("%s  物体 %zu  弾 %zu  %.0f FPS", isPlaying_ ? (isPaused_ ? "一時停止中" : "再生中") : "編集中",
				objects_.size(), projectiles_.size(), ImGui::GetIO().Framerate);

	ImGui::TextDisabled("左クリック: 選ぶ  右ドラッグ: 回す  中ドラッグ: 動かす  ホイール: 近づく  Enter: Play / Stop  Space（Play 中）: 球を撃つ");
	ImGui::End();
}


void ou::PhysicsEditor::ShowHierarchy() {
	ImGui::SetNextWindowPos({10.0f, 130.0f}, ImGuiCond_FirstUseEver);
	ImGui::SetNextWindowSize({260.0f, 420.0f}, ImGuiCond_FirstUseEver);
	if (!ImGui::Begin("Hierarchy")) {
		ImGui::End();
		return;
	}

	// 置くのは編集中だけ
	ImGui::BeginDisabled(isPlaying_);
	const auto spawnPosition = cameraTarget_ + Vector3{0.0f, 4.0f, 0.0f};
	if (ImGui::Button("+ 箱")) {
		specs_.push_back(MakeBox("Box", spawnPosition, Vector3::One, {0.3f, 0.6f, 0.9f, 1.0f}));
		selected_ = static_cast<int>(specs_.size()) - 1;
		isDirty_ = true;
	}
	ImGui::SameLine();
	if (ImGui::Button("+ 球")) {
		specs_.push_back(MakeSphere("Sphere", spawnPosition, 1.0f, {0.9f, 0.45f, 0.2f, 1.0f}));
		selected_ = static_cast<int>(specs_.size()) - 1;
		isDirty_ = true;
	}
	ImGui::SameLine();
	if (ImGui::Button("+ 壁")) {
		auto wall = MakeBox("Wall", cameraTarget_ + Vector3{0.0f, 1.0f, 0.0f}, {6.0f, 2.0f, 0.5f}, {0.5f, 0.5f, 0.55f, 1.0f});
		wall.bodyType = BodyType::Static;
		specs_.push_back(wall);
		selected_ = static_cast<int>(specs_.size()) - 1;
		isDirty_ = true;
	}

	const auto hasSelection = (selected_ >= 0 && selected_ < static_cast<int>(specs_.size()));
	ImGui::BeginDisabled(!hasSelection);
	if (ImGui::Button("複製") && hasSelection) {
		auto copy = specs_[selected_];
		copy.name += " (copy)";
		copy.position += Vector3{1.0f, 1.0f, 0.0f};
		specs_.push_back(copy);
		selected_ = static_cast<int>(specs_.size()) - 1;
		isDirty_ = true;
	}
	ImGui::SameLine();
	if (ImGui::Button("削除") && hasSelection) {
		specs_.erase(specs_.begin() + selected_);
		selected_ = -1;
		isDirty_ = true;
	}
	ImGui::EndDisabled();
	ImGui::EndDisabled();

	ImGui::Separator();
	for (size_t i = 0; i < specs_.size(); i++) {
		ImGui::PushID(static_cast<int>(i));
		const auto &spec = specs_[i];
		const auto label = std::format("{} [{}{}]", spec.name, spec.bodyType == BodyType::Static ? "S" : spec.bodyType == BodyType::Dynamic ? "D" : "K",
									   spec.isTrigger ? " T" : "");
		if (ImGui::Selectable(label.c_str(), selected_ == static_cast<int>(i))) {
			selected_ = static_cast<int>(i);
		}
		ImGui::PopID();
	}

	ImGui::End();
}


void ou::PhysicsEditor::ShowInspector() {
	ImGui::SetNextWindowPos({static_cast<float>(Screen::GetWidth()) - 380.0f, 10.0f}, ImGuiCond_FirstUseEver);
	ImGui::SetNextWindowSize({370.0f, 620.0f}, ImGuiCond_FirstUseEver);
	if (!ImGui::Begin("Inspector")) {
		ImGui::End();
		return;
	}

	if (selected_ < 0 || selected_ >= static_cast<int>(specs_.size())) {
		ImGui::TextDisabled("物体を選んでください（左クリックか Hierarchy）");
		ImGui::End();
		return;
	}

	if (isPlaying_) {
		ShowLiveState(objects_[selected_]);
	} else if (EditSpec(specs_[selected_])) {
		isDirty_ = true;
	}

	ImGui::End();
}


bool ou::PhysicsEditor::EditSpec(BodySpec &spec) {
	auto isChanged = false;

	isChanged |= ImGui::InputText("名前", &spec.name);

	auto shape = static_cast<int>(spec.shape);
	if (ImGui::Combo("形", &shape, kShapeNames, static_cast<int>(std::size(kShapeNames)))) {
		spec.shape = static_cast<ShapeType>(shape);
		isChanged = true;
	}
	auto bodyType = static_cast<int>(spec.bodyType);
	if (ImGui::Combo("種類", &bodyType, kBodyTypeNames, static_cast<int>(std::size(kBodyTypeNames)))) {
		spec.bodyType = static_cast<BodyType>(bodyType);
		isChanged = true;
	}
	isChanged |= ImGui::Checkbox("isTrigger", &spec.isTrigger);

	ImGui::SeparatorText("Transform");
	isChanged |= ImGui::DragFloat3("位置", &spec.position.x, 0.05f);
	isChanged |= ImGui::DragFloat3("回転（度）", &spec.rotation.x, 0.5f);
	if (spec.shape == ShapeType::Sphere) {
		isChanged |= ImGui::DragFloat("直径", &spec.scale.x, 0.02f, 0.05f, 50.0f);
	} else {
		isChanged |= ImGui::DragFloat3("大きさ", &spec.scale.x, 0.02f, 0.05f, 100.0f);
	}
	isChanged |= ImGui::ColorEdit3("色", &spec.color.r);

	if (spec.bodyType != BodyType::Static) {
		ImGui::SeparatorText("Rigidbody");
		isChanged |= ImGui::DragFloat("mass", &spec.mass, 0.05f, 0.01f, 1000.0f);
		isChanged |= ImGui::Checkbox("useGravity", &spec.useGravity);
		isChanged |= ImGui::DragFloat("linearDamping", &spec.linearDamping, 0.01f, 0.0f, 50.0f);
		isChanged |= ImGui::DragFloat("angularDamping", &spec.angularDamping, 0.01f, 0.0f, 50.0f);
		isChanged |= ImGui::DragFloat3("初速", &spec.initialVelocity.x, 0.1f);

		ImGui::Text("constraints");
		for (int i = 0; i < 6; i++) {
			if (i % 3 != 0) { ImGui::SameLine(); }
			isChanged |= ImGui::Checkbox(kFreezeNames[i], &spec.freeze[i]);
		}

		if (spec.bodyType == BodyType::Kinematic) {
			isChanged |= ImGui::Checkbox("左右に往復（MovePosition）", &spec.isMoving);
		}
	}

	return isChanged;
}


void ou::PhysicsEditor::ShowLiveState(GameObject *object) {
	const auto &spec = specs_[selected_];
	ImGui::Text("%s（再生中は見るだけ。Stop で元に戻る）", spec.name.c_str());

	const auto *transform = object->GetTransform();
	ImGui::Text("位置 %s", FormatVector(transform->GetWorldPosition()).c_str());
	ImGui::Text("回転 %s 度", FormatVector(transform->GetWorldRotation().EulerAngles() * Mathf::Rad2Deg).c_str());

	if (const auto *reporter = object->GetComponent<ContactReporter>()) {
		ImGui::Text("触れている数 %d", reporter->GetTouchingCount());
	}

	auto *rigidbody = object->GetComponent<Rigidbody>();
	if (!rigidbody) {
		ImGui::TextDisabled("Rigidbody なし（Static）");
		return;
	}

	ImGui::SeparatorText("Rigidbody");
	ImGui::Text("linearVelocity %s", FormatVector(rigidbody->GetLinearVelocity()).c_str());
	ImGui::Text("angularVelocity %s", FormatVector(rigidbody->GetAngularVelocity()).c_str());
	ImGui::Text("isKinematic %s  mass %.2f", rigidbody->IsKinematic() ? "true" : "false", rigidbody->GetMass());

	ImGui::SeparatorText("力を加える（VelocityChange）");
	if (ImGui::Button("上に跳ねる")) { rigidbody->AddForce({0.0f, 6.0f, 0.0f}, ForceMode::VelocityChange); }
	ImGui::SameLine();
	if (ImGui::Button("奥へ押す")) {
		auto forward = camera_->GetTransform()->GetForward();
		forward.y = 0.0f;
		rigidbody->AddForce(forward.Normalize() * 6.0f, ForceMode::VelocityChange);
	}
	ImGui::SameLine();
	if (ImGui::Button("回す")) { rigidbody->AddTorque({0.0f, 10.0f, 0.0f}, ForceMode::VelocityChange); }
}


void ou::PhysicsEditor::ShowSettings() {
	ImGui::SetNextWindowPos({10.0f, 560.0f}, ImGuiCond_FirstUseEver);
	ImGui::SetNextWindowSize({260.0f, 170.0f}, ImGuiCond_FirstUseEver);
	if (!ImGui::Begin("Physics Settings")) {
		ImGui::End();
		return;
	}

	auto gravity = Physics::GetGravity();
	if (ImGui::DragFloat3("gravity", &gravity.x, 0.05f)) { Physics::SetGravity(gravity); }
	if (ImGui::Button("重力を戻す")) { Physics::SetGravity({0.0f, -9.81f, 0.0f}); }

	auto fixedDeltaTime = Time::GetFixedDeltaTime();
	if (ImGui::DragFloat("fixedDeltaTime", &fixedDeltaTime, 0.001f, 0.002f, 0.1f, "%.3f")) {
		Time::SetFixedDeltaTime(fixedDeltaTime);
	}

	auto queriesHitTriggers = Physics::GetQueriesHitTriggers();
	if (ImGui::Checkbox("queriesHitTriggers", &queriesHitTriggers)) {
		Physics::SetQueriesHitTriggers(queriesHitTriggers);
	}

	ImGui::DragFloat("弾の速さ", &shootSpeed_, 0.5f, 1.0f, 100.0f);
	ImGui::End();
}


void ou::PhysicsEditor::ShowEventLog() {
	ImGui::SetNextWindowPos({static_cast<float>(Screen::GetWidth()) - 380.0f, 640.0f}, ImGuiCond_FirstUseEver);
	ImGui::SetNextWindowSize({370.0f, 240.0f}, ImGuiCond_FirstUseEver);
	if (!ImGui::Begin("Event Log")) {
		ImGui::End();
		return;
	}

	if (ImGui::Button("消す")) { sEventLog.clear(); }
	ImGui::Separator();

	ImGui::BeginChild("##log");
	for (const auto &line : sEventLog) {
		ImGui::TextUnformatted(line.c_str());
	}
	// 一番下を見ているときは、新しい行に付いていく
	if (ImGui::GetScrollY() >= ImGui::GetScrollMaxY()) { ImGui::SetScrollHereY(1.0f); }
	ImGui::EndChild();

	ImGui::End();
}

#else

void ou::PhysicsEditor::ShowToolbar() {}
void ou::PhysicsEditor::ShowHierarchy() {}
void ou::PhysicsEditor::ShowInspector() {}
void ou::PhysicsEditor::ShowLiveState(GameObject *) {}
void ou::PhysicsEditor::ShowSettings() {}
void ou::PhysicsEditor::ShowEventLog() {}
bool ou::PhysicsEditor::EditSpec(BodySpec &) { return false; }

#endif


// ----- ContactReporter -----

void ou::ContactReporter::OnCollisionEnter(const Collision &collision) {
	touchingCount_++;

	// 両方に届くので、片方だけが書く
	const auto *other = collision.GetCollider()->GetGameObject();
	if (GetGameObject()->GetInstanceId() > other->GetInstanceId()) { return; }

	const auto &contact = collision.GetContactPoint(0);
	AddEventLog(std::format("{:6.2f} OnCollisionEnter {} - {}  法線 {}", Time::GetTime(), GetGameObject()->GetName(), other->GetName(),
							FormatVector(contact.normal)));
}


void ou::ContactReporter::OnCollisionExit(const Collision &collision) {
	touchingCount_--;

	const auto *other = collision.GetCollider()->GetGameObject();
	if (GetGameObject()->GetInstanceId() > other->GetInstanceId()) { return; }
	AddEventLog(std::format("{:6.2f} OnCollisionExit  {} - {}", Time::GetTime(), GetGameObject()->GetName(), other->GetName()));
}


void ou::ContactReporter::OnTriggerEnter(Collider *other) {
	touchingCount_++;

	if (GetGameObject()->GetInstanceId() > other->GetGameObject()->GetInstanceId()) { return; }
	AddEventLog(std::format("{:6.2f} OnTriggerEnter   {} - {}", Time::GetTime(), GetGameObject()->GetName(), other->GetGameObject()->GetName()));
}


void ou::ContactReporter::OnTriggerExit(Collider *other) {
	touchingCount_--;

	if (GetGameObject()->GetInstanceId() > other->GetGameObject()->GetInstanceId()) { return; }
	AddEventLog(std::format("{:6.2f} OnTriggerExit    {} - {}", Time::GetTime(), GetGameObject()->GetName(), other->GetGameObject()->GetName()));
}


int ou::ContactReporter::GetTouchingCount() const {
	return touchingCount_;
}


// ----- KinematicMover -----

void ou::KinematicMover::Start() {
	origin_ = GetTransform()->GetWorldPosition();
}


void ou::KinematicMover::FixedUpdate() {
	auto *rigidbody = GetGameObject()->GetComponent<Rigidbody>();
	if (!rigidbody) { return; }

	time_ += Time::GetDeltaTime();
	rigidbody->MovePosition(origin_ + Vector3{std::sin(time_) * 5.0f, 0.0f, 0.0f});
}
