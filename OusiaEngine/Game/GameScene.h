#pragma once

#include "Math/Color.h"
#include "Math/Vector2.h"
#include "Math/Vector3.h"
#include "Scene/Component.h"
#include "Scene/Scene.h"

#include <memory>
#include <string>
#include <vector>


namespace ou {

// 前方宣言
class Camera;
class GameObject;
class Mesh;

void BuildGameScene(Scene &scene);

// 物理を試すための簡易エディタ
// 物体を置いて Play で物理を動かし、Stop で置いたときの状態に戻す（Unity の Play モードと同じ）
class PhysicsEditor final: public Component {
public: // インナークラス

	enum class ShapeType {
		Box,
		Sphere,
	};

	enum class BodyType {
		Static,    // Rigidbody なし
		Dynamic,   // Rigidbody
		Kinematic, // isKinematic の Rigidbody
	};

	// 置く物体 1 つ。Stop のたびにここから作り直す
	struct BodySpec {
		std::string name = "Object";
		ShapeType shape = ShapeType::Box;
		BodyType bodyType = BodyType::Dynamic;
		Vector3 position = Vector3::Zero;
		Vector3 rotation = Vector3::Zero; // 度
		Vector3 scale = Vector3::One;     // 箱の大きさ。球は x を直径にする
		Color color = {0.8f, 0.8f, 0.8f, 1.0f};
		bool isTrigger = false;
		float mass = 1.0f;
		bool useGravity = true;
		float linearDamping = 0.0f;
		float angularDamping = 0.05f;
		bool freeze[6] = {}; // 位置 x / y / z、回転 x / y / z
		Vector3 initialVelocity = Vector3::Zero;
		bool isMoving = false; // Kinematic を MovePosition で左右に往復させる
	};


public: // メンバ関数

	void Start() override;
	void Update() override;


private: // メンバ変数

	std::vector<BodySpec> specs_;
	std::vector<GameObject *> objects_;     // specs_ と同じ順
	std::vector<GameObject *> projectiles_; // Play 中に撃った球（Stop で消す）
	int selected_ = -1;
	bool isDirty_ = false; // 仕様が変わったので作り直す

	bool isPlaying_ = false;
	bool isPaused_ = false;
	bool isStepRequested_ = false;
	float playSpeed_ = 1.0f;
	float shootSpeed_ = 20.0f;
	int presetIndex_ = 0;

	Camera *camera_ = nullptr;
	Vector3 cameraTarget_ = {0.0f, 1.0f, 0.0f};
	float cameraYaw_ = 0.0f;    // 度
	float cameraPitch_ = 25.0f; // 度
	float cameraDistance_ = 22.0f;

	std::shared_ptr<Mesh> cubeMesh_;
	std::shared_ptr<Mesh> sphereMesh_;


private: // メンバ関数

	void LoadPreset(int index);
	void Rebuild();
	GameObject *Spawn(const BodySpec &spec);

	void Play();
	void Stop();
	void UpdateTimeScale();

	void UpdateCamera();
	void HandleMouseAndKeys();
	void ShootBall(const Vector3 &origin, const Vector3 &direction);
	bool ScreenPointToRay(const Vector2 &screenPoint, Vector3 *origin, Vector3 *direction) const;
	void SelectByRaycast(const Vector2 &screenPoint);
	void UpdateHighlight();

	void ShowToolbar();
	void ShowHierarchy();
	void ShowInspector();
	void ShowLiveState(GameObject *object);
	void ShowSettings();
	void ShowEventLog();
	bool EditSpec(BodySpec &spec);
};

// 接触のイベントを PhysicsEditor のログに送り、今触れている数を数える
class ContactReporter final: public Component {
public: // メンバ関数

	void OnCollisionEnter(const Collision &collision) override;
	void OnCollisionExit(const Collision &collision) override;
	void OnTriggerEnter(Collider *other) override;
	void OnTriggerExit(Collider *other) override;

	int GetTouchingCount() const;


private: // メンバ変数

	int touchingCount_ = 0;
};

// Kinematic の Rigidbody を MovePosition で左右に往復させる
class KinematicMover final: public Component {
public: // メンバ関数

	void Start() override;
	void FixedUpdate() override;


private: // メンバ変数

	Vector3 origin_ = Vector3::Zero;
	float time_ = 0.0f;
};

} // namespace ou
