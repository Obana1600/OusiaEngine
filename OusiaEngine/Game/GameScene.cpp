#include "GameScene.h"
#include "Core/Time.h"
#include "Diagnostics/Log.h"
#include "Math/Mathf.h"
#include "Physics/BoxCollider.h"
#include "Physics/Rigidbody.h"
#include "Physics/SphereCollider.h"
#include "Render/Camera.h"
#include "Scene/GameObject.h"
#include "Scene/Transform.h"

using namespace ou;


namespace {

// 補間が見えるように、ステップを大きくする
constexpr float kFixedDeltaTime = 0.1f;
constexpr float kSpeed = 2.0f;

Rigidbody *CreateMover(Scene &scene, const char *name, float z, RigidbodyInterpolation interpolation) {
	auto *object = scene.CreateGameObject(name);
	object->GetTransform()->SetWorldPosition({0.0f, 5.0f, z});
	object->AddComponent<BoxCollider>()->SetSize({0.5f, 0.5f, 0.5f});
	auto *rigidbody = object->AddComponent<Rigidbody>();
	rigidbody->SetUseGravity(false);
	rigidbody->SetInterpolation(interpolation);
	rigidbody->SetLinearVelocity({kSpeed, 0.0f, 0.0f});
	return rigidbody;
}

float X(const Rigidbody *rigidbody) {
	return rigidbody->GetTransform()->GetWorldPosition().x;
}

float Max(float a, float b) {
	return (a > b) ? a : b;
}

// FixedUpdate で Transform と Rigidbody の位置の差を見る
class FixedUpdateWatcher final: public Component {
public:
	float gap = 0.0f;

	void FixedUpdate() override {
		auto *rigidbody = GetGameObject()->GetComponent<Rigidbody>();
		gap = Max(gap, Mathf::Abs(X(rigidbody) - rigidbody->GetPosition().x));
	}
};

} // namespace



void ou::BuildGameScene(Scene &scene) {
	Time::SetFixedDeltaTime(kFixedDeltaTime);
	scene.CreateGameObject("Main Camera")->AddComponent<Camera>();

	auto *floor = scene.CreateGameObject("Floor");
	floor->GetTransform()->SetWorldPosition({0.0f, -0.5f, 0.0f});
	floor->AddComponent<BoxCollider>()->SetSize({60.0f, 1.0f, 40.0f});

	auto *probe = scene.CreateGameObject("Probe")->AddComponent<InterpolationProbe>();
	probe->none = CreateMover(scene, "None", -6.0f, RigidbodyInterpolation::None);
	probe->interpolate = CreateMover(scene, "Interpolate", -3.0f, RigidbodyInterpolation::Interpolate);
	probe->interpolate->GetGameObject()->AddComponent<FixedUpdateWatcher>();
	probe->extrapolate = CreateMover(scene, "Extrapolate", 0.0f, RigidbodyInterpolation::Extrapolate);

	// y 軸まわりに 1 rad/s で回す
	probe->spinner = CreateMover(scene, "Spinner", 3.0f, RigidbodyInterpolation::Interpolate);
	probe->spinner->SetLinearVelocity(Vector3::Zero);
	probe->spinner->SetAngularVelocity({0.0f, 1.0f, 0.0f});
	probe->spinner->SetAngularDamping(0.0f);

	// 1.5 秒で Transform から x 20 に移す
	probe->teleported = CreateMover(scene, "Teleported", 6.0f, RigidbodyInterpolation::Interpolate);

	// 床に落として止める
	auto *ball = scene.CreateGameObject("Resting");
	ball->GetTransform()->SetWorldPosition({-10.0f, 2.0f, 0.0f});
	ball->AddComponent<SphereCollider>()->SetRadius(0.3f);
	probe->resting = ball->AddComponent<Rigidbody>();
	probe->resting->SetInterpolation(RigidbodyInterpolation::Interpolate);
}


void ou::InterpolationProbe::Update() {
	const auto time = Time::GetTime();

	if (time >= 0.5f && time < 1.5f) {
		// 最後のステップの時刻 fixedTime のとき、Body は kSpeed * fixedTime にいる
		noneError_ = Max(noneError_, Mathf::Abs(X(none) - kSpeed * Time::GetFixedTime()));
		interpolateError_ = Max(interpolateError_, Mathf::Abs(X(interpolate) - kSpeed * (time - kFixedDeltaTime)));
		extrapolateError_ = Max(extrapolateError_, Mathf::Abs(X(extrapolate) - kSpeed * time));

		Vector3 axis;
		float angle = 0.0f;
		spinner->GetTransform()->GetWorldRotation().ToAngleAxis(&axis, &angle);
		spinnerError_ = Max(spinnerError_, Mathf::Abs(angle - (time - kFixedDeltaTime)));

		if (frames_ > 0) {
			noneMoved_ += (X(none) != lastNoneX_) ? 1 : 0;
			interpolateMoved_ += (X(interpolate) != lastInterpolateX_) ? 1 : 0;
		}
		frames_++;
		lastNoneX_ = X(none);
		lastInterpolateX_ = X(interpolate);
	}

	if (step_ == 0 && time >= 1.5f) {
		step_++;
		teleported->GetTransform()->SetWorldPosition({20.0f, 5.0f, 6.0f});
	} else if (step_ == 1) {
		// 移したあと、元の場所（x 3 くらい）との間に戻らない
		teleportMinX_ = Mathf::Min(teleportMinX_, X(teleported));
		if (time >= 2.0f) {
			step_++;
			Log::Info("TEST 移したあと 一番小さい x {:.3f} / Body の x {:.3f} / 0.5 秒で進むはず 21.0", teleportMinX_, teleported->GetPosition().x);
		}
	}

	if (time >= 3.0f && time < 4.0f) {
		restingGap_ = Max(restingGap_, Mathf::Abs(resting->GetTransform()->GetWorldPosition().y - resting->GetPosition().y));
	}

	if (step_ == 2 && time >= 4.0f) {
		step_++;
		Log::Info("TEST フレーム {} / 動いたフレーム None {} Interpolate {}", frames_ - 1, noneMoved_, interpolateMoved_);
		Log::Info("TEST 計算との差 None {:.5f} / Interpolate {:.5f} / Extrapolate {:.5f} / 回転 {:.5f} rad", noneError_, interpolateError_, extrapolateError_, spinnerError_);
		Log::Info("TEST FixedUpdate での Transform と Body の差 Interpolate {:.3f}（1 ステップ以内 = {:.1f} 以下）",
				  interpolate->GetGameObject()->GetComponent<FixedUpdateWatcher>()->gap, kSpeed * kFixedDeltaTime);
		Log::Info("TEST 止まった玉 Transform と Body の y の差 {:.6f} / y {:.3f}", restingGap_, resting->GetPosition().y);
	}
}
