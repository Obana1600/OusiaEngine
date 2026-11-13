#pragma once

#include "Editor/EditorWindow.h"
#include "Editor/SceneRenderTarget.h"
#include "Math/Quaternion.h"
#include "Math/Vector2.h"
#include "Math/Vector3.h"
#include "Render/Camera.h"


namespace ou {

class SceneView final: public EditorWindow {
public: // 静的メンバ関数

	static SceneView *FindVisible();


public: // メンバ関数

	SceneView();

	const SceneRenderTarget &GetRenderTarget() const;

	Camera::PerCamera MakePerCamera() const;


protected: // メンバ関数

	void OnGui() override;


private: // インナークラス

	enum class DragMode {
		None,
		Fly,
		Pan,
		Orbit,
	};


private: // メンバ変数

	SceneRenderTarget renderTarget_{"SceneView"};

	Vector3 pivot_{0.0f, 0.5f, 0.0f};
	float pitch_ = 20.0f;
	float yaw_ = 0.0f;
	float distance_ = 6.0f;
	float flySpeed_ = 5.0f;

	DragMode dragMode_ = DragMode::None;


private: // メンバ関数

	void HandleInput(bool isHovered);
	void StartDrag();

	void Fly(const Vector2 &mouseDelta);
	void Pan(const Vector2 &mouseDelta);
	void Orbit(const Vector2 &mouseDelta);
	void Zoom(float wheel);
	void FrameSelected();

	Quaternion MakeRotation() const;
	Vector3 MakePosition() const;
};

} // namespace ou
