#pragma once

#include "Camera.h"
#include "Math/Vector3.h"
#include "base/Window.h"

class InputSystem;


namespace ou {

class DebugCamera {
public: // public関数

	void Initialize(
		int windowWidth = 1280,
		int windowHeight = 720);

	void Update();


	// accesser ------------------------------------------------------------

	const Camera &GetCamera() const {
		return camera_;
	}

	void SetFovAngleY(float value) {
		camera_.fovAngleY_ = value;
	}

	void SetAspectRatio(float value) {
		camera_.aspectRatio_ = value;
	}

	void SetNearZ(float value) {
		camera_.nearZ_ = value;
	}

	void SetFarZ(float value) {
		camera_.farZ_ = value;
	}

	// -------------------------------------------------------------------


private: // メンバ変数

	Camera camera_{};

	Vector3 pivot_ = {0.0f, 0.0f, 0.0f};
	float yaw_ = 0.0f;
	float pitch_ = 0.0f;
	float distance_ = 5.0f;

	InputSystem *input_ = nullptr;


private: // メンバ関数

	void UpdateTransform();
	void UpdateMatrix();
};

} // namespace ou