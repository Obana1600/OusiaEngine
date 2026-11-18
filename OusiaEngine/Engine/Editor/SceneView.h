#pragma once

#include "Editor/Editor.h"
#include "Editor/EditorWindow.h"
#include "Editor/SceneRenderTarget.h"
#include "Math/Quaternion.h"
#include "Math/Vector2.h"
#include "Math/Vector3.h"
#include "Render/Camera.h"

#include <cstdint>
#include <memory>
#include <vector>


namespace ou {

// 前方宣言
class GameObject;

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

	// 左ボタンを、Scene の絵の上（ハンドルの外）で押した（離したときに、動かしていなければ選ぶ）
	bool isSelectPressed_ = false;

	// 選んでいる GameObject のコンポーネントの Editor（OnSceneGui を呼ぶ）
	uint32_t sceneGuiTargetId_ = 0;
	std::vector<uint32_t> sceneGuiComponentIds_;
	std::vector<std::unique_ptr<Editor>> sceneGuiEditors_;


private: // メンバ関数

	void DrawToolbar();
	void SelectToolByKey();

	bool DrawTransformHandle();
	void DrawSceneGuis(const Rect &sceneRect);

	void HandleInput(bool isHovered);

	// 左クリックで、マウスの下のものを選ぶ（何もなければ選択を外す）
	void HandleClickSelection(bool isHovered, const Vector2 &imageMin, const Vector2 &imageSize);
	// アイコン → メッシュの箱（OBB）の順に当て、いちばん手前のものを返す
	GameObject *PickGameObject(const Vector2 &mouse, const Vector2 &imageMin, const Vector2 &imageSize) const;
	void StartDrag();

	void Fly(const Vector2 &mouseDelta);
	void Pan(const Vector2 &mouseDelta);
	void Orbit(const Vector2 &mouseDelta);
	void Zoom(float wheel);

	// Project から落としたモデル・プレハブを、マウスの先に置く
	void AcceptAssetDrop();
	void FrameSelected();

	Quaternion MakeRotation() const;
	Vector3 MakePosition() const;
	// 画面の mouse を通る、カメラからのレイの向き（長さ 1）
	Vector3 MakeRayDirection(const Vector2 &mouse, const Vector2 &imageMin, const Vector2 &imageSize) const;
};

} // namespace ou
