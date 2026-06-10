#include "DebugCamera.h"
#include "math/MathUtility.h"
#include "base/WinApp.h"
#include <input/InputManager.h>
#include <externals/imgui/imgui.h>


using namespace MathUtility;

// public func ====================================================================================

void DebugCamera::Initialize() {
}


void DebugCamera::Update() {
	UpdateTransform();
	UpdateMatrix();
}

// ==================================================================================== public func



// private func ===================================================================================

void DebugCamera::UpdateTransform() {
	if (ImGui::GetIO().WantCaptureMouse) {
		return;
	}

	InputManager *input = InputManager::GetInstance();

	// ホイールで前後移動
	int32_t wheel = input->GetWheel();
	if (wheel != 0) {
		m_distance -= static_cast<float>(wheel) * 0.01f;
		if (m_distance < 0.1f) {
			m_distance = 0.1f;
		}
	}

	// ミドルボタン押下
	if (input->PressMouse(InputManager::MIDDLE)) {
		InputManager::MouseMove move = input->GetMouseMove();

		if (input->PushKey(DIK_LSHIFT) || input->PushKey(DIK_RSHIFT)) {
			// 上下左右移動
			float panSpeed = 0.001f * m_distance;
			Vector3 right = { m_matRot.m[0][0], m_matRot.m[0][1], m_matRot.m[0][2] };
			Vector3 up = { m_matRot.m[1][0], m_matRot.m[1][1], m_matRot.m[1][2] };

			m_pivot += -right * (static_cast<float>(move.x) * panSpeed) + up * (static_cast<float>(move.y) * panSpeed);
		} else {
			// ピボット回転
			float rotSpeed = 0.005f;
			float deltaX = static_cast<float>(move.x) * rotSpeed;
			float deltaY = static_cast<float>(move.y) * rotSpeed;

			Matrix4x4 matRotDelta = MakeRotateXMatrix(deltaY) * MakeRotateYMatrix(deltaX);
			m_matRot *= matRotDelta;
		}
	}

	// rボタン
	if (input->TriggerKey(DIK_R)) {
		m_pivot = { 0.0f, 0.0f, 0.0f };
		m_matRot = MathUtility::MakeIdentity4x4();
		m_distance = 5.0f;
	}
}


void DebugCamera::UpdateMatrix() {
	Vector3 forward = { m_matRot.m[2][0], m_matRot.m[2][1], m_matRot.m[2][2] };
	Vector3 cameraPos = m_pivot - forward * m_distance;

	Matrix4x4 cameraMatrix = m_matRot * MakeTranslateMatrix(cameraPos);
	Matrix4x4 viewMatrix = Inverse(cameraMatrix);
	Matrix4x4 projectionMatrix =
		MakePerspectiveFovMatrix(0.45f, float(WinApp::kWindowWidth) / float(WinApp::kWindowHeight), 0.1f, 1000.0f);

	m_viewProjectionMatrix = viewMatrix * projectionMatrix;
	m_viewportMatrix = MakeViewportMatrix(0.0f, 0.0f, float(WinApp::kWindowWidth), float(WinApp::kWindowHeight), 0.0f, 1.0f);
}


// =================================================================================== private func