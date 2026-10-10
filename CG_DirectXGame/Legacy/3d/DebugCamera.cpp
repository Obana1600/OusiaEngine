#include "DebugCamera.h"

#include "base/Window.h"
#include "externals/imgui/imgui.h"
#include "input/InputSystem.h"
#include "math/Matrix4x4.h"
#include <algorithm>
#include <cmath>

using namespace ou;


void DebugCamera::Initialize(
	int windowWidth,
	int windowHeight
) {
	input_ = InputSystem::GetInstance();

	camera_.Initialize();
	camera_.aspectRatio_ = static_cast<float>(windowWidth) / static_cast<float>(windowHeight);
}


void DebugCamera::Update() {
	UpdateTransform();
	UpdateMatrix();
}


void DebugCamera::UpdateTransform() {
	if (ImGui::GetIO().WantCaptureMouse) {
		return;
	}

	// ホイールで前後移動
	int32_t wheel = input_->GetWheel();
	if (wheel != 0) {
		distance_ *= std::pow(0.9f, static_cast<float>(wheel) / 120.0f);
		if (distance_ < 0.1f) {
			distance_ = 0.1f;
		}
	}

	// ミドルボタン押下
	if (input_->PressMouse(InputSystem::MIDDLE)) {
		auto move = input_->GetMouseMove();

		if (input_->PushKey(DIK_LSHIFT) || input_->PushKey(DIK_RSHIFT)) {
			// 上下左右移動
			float sy = std::sin(yaw_);
			float cy = std::cos(yaw_);
			float sp = std::sin(pitch_);
			float cp = std::cos(pitch_);

			Vector3 right = {cy, 0.0f, -sy};
			Vector3 up = {sp * sy, cp, sp * cy};

			float panSpeed = 0.001f * distance_;
			pivot_ += -right * (static_cast<float>(move.x) * panSpeed) + up * (static_cast<float>(move.y) * panSpeed);

		} else {
			// ピボット回転
			constexpr float rotSpeed = 0.005f;
			yaw_ += static_cast<float>(move.x) * rotSpeed;
			pitch_ += static_cast<float>(move.y) * rotSpeed;

			constexpr float kPitchLimit = std::numbers::pi_v<float> / 2.0f - 0.01f;
			pitch_ = std::clamp(pitch_, -kPitchLimit, kPitchLimit);
		}
	}

	// rボタン
	if (input_->TriggerKey(DIK_R)) {
		pivot_ = {0.0f, 0.0f, 0.0f};
		yaw_ = 0.0f;
		pitch_ = 0.0f;
		distance_ = 5.0f;
	}
}


void DebugCamera::UpdateMatrix() {
	float sy = std::sin(yaw_);
	float cy = std::cos(yaw_);
	float sp = std::sin(pitch_);
	float cp = std::cos(pitch_);

	Vector3 forward = {cp * sy, -sp, cp * cy};
	camera_.translation_ = pivot_ - forward * distance_;
	camera_.rotation_ = {pitch_, yaw_, 0.0f};

	camera_.Update();
}
