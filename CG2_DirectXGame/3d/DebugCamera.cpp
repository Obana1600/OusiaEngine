#include "DebugCamera.h"
#include "math/MathUtility.h"
#include "base/WinApp.h"


using namespace MathUtility;

// public func ====================================================================================

void DebugCamera::Initialize() {
}


void DebugCamera::Update() {
	Matrix4x4 cameraMatrix = MakeAffineMatrix({1.0f, 1.0f, 1.0f}, m_rotation, m_translation);
	Matrix4x4 viewMatrix = Inverse(cameraMatrix);
	Matrix4x4 projectionMatrix = 
		MakePerspectiveFovMatrix(0.45f, float(WinApp::kWindowWidth) / float(WinApp::kWindowHeight), 0.1f, 1000.0f);

	m_viewProjectionMatrix = viewMatrix * projectionMatrix;
	m_viewportMatrix = MakeViewportMatrix(0.0f, 0.0f, float(WinApp::kWindowWidth), float(WinApp::kWindowHeight), 0.0f, 1.0f);
}

// ==================================================================================== public func



// private func ===================================================================================



// =================================================================================== private func