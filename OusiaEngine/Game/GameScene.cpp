#include "GameScene.h"
#include "Core/Time.h"
#include "Diagnostics/Log.h"
#include "Math/Mathf.h"
#include "Math/Quaternion.h"
#include "Render/Camera.h"
#include "Render/LineRenderer.h"
#include "Scene/GameObject.h"
#include "Scene/Transform.h"

#include <algorithm>
#include <cmath>
#include <vector>

using namespace ou;


namespace {

// 壁（xy 平面の箱）の半分の大きさ
constexpr float kWallHalfWidth = 8.0f;
constexpr float kWallHalfHeight = 5.0f;

// origin から direction に進み、壁で反射しながら length だけ進んだ折れ線
std::vector<Vector3> TraceReflection(Vector3 origin, Vector3 direction, float length, int maxBounces) {
	std::vector<Vector3> points = {origin};
	for (int bounce = 0; bounce <= maxBounces && length > 0.0f; bounce++) {
		// 先に当たる壁までの距離
		const auto tx = direction.x > 0.0f ? (kWallHalfWidth - origin.x) / direction.x
					  : direction.x < 0.0f ? (-kWallHalfWidth - origin.x) / direction.x
										   : Mathf::Infinity;
		const auto ty = direction.y > 0.0f ? (kWallHalfHeight - origin.y) / direction.y
					  : direction.y < 0.0f ? (-kWallHalfHeight - origin.y) / direction.y
										   : Mathf::Infinity;
		const auto t = Mathf::Min(tx, ty, length);

		origin = origin + direction * t;
		points.push_back(origin);
		length -= t;

		if (tx <= ty) { direction.x = -direction.x; }
		if (ty <= tx) { direction.y = -direction.y; }
	}
	return points;
}

} // namespace



void ou::BuildGameScene(Scene &scene) {
	scene.CreateGameObject("Line Ring Test")->AddComponent<LineRingTest>();
}


void ou::LineRingTest::Start() {
	auto &scene = *GetGameObject()->GetScene();

	// xy 平面を正面から見る
	auto *cameraObject = scene.CreateGameObject("Main Camera");
	cameraObject->GetTransform()->SetLocalPosition({0.0f, 0.0f, -16.0f});
	cameraObject->AddComponent<Camera>();

	// 壁（loop）
	auto *walls = scene.CreateGameObject("Walls")->AddComponent<LineRenderer>();
	const Vector3 corners[] = {
		{-kWallHalfWidth, -kWallHalfHeight, 0.0f},
		{kWallHalfWidth, -kWallHalfHeight, 0.0f},
		{kWallHalfWidth, kWallHalfHeight, 0.0f},
		{-kWallHalfWidth, kWallHalfHeight, 0.0f},
	};
	walls->SetPositions(corners);
	walls->SetLoop(true);
	walls->SetWidthMultiplier(0.2f);
	walls->SetStartColor(Color::Gray);
	walls->SetEndColor(Color::Gray);

	// 予測線（毎フレーム頂点が変わる。先に行くほど細く透明に）
	predictionLine_ = scene.CreateGameObject("Prediction Line")->AddComponent<LineRenderer>();
	predictionLine_->SetStartWidth(0.25f);
	predictionLine_->SetEndWidth(0.05f);
	predictionLine_->SetStartColor(Color::White);
	predictionLine_->SetEndColor({0.3f, 0.8f, 1.0f, 0.0f});

	// 予兆線（ローカル座標。Transform の回転に付いてくる）
	auto *warningObject = scene.CreateGameObject("Warning Line");
	warningObject->GetTransform()->SetLocalPosition({4.0f, 1.5f, 0.0f});
	warningLine_ = warningObject->AddComponent<LineRenderer>();
	const Vector3 warningPoints[] = {Vector3::Zero, {6.0f, 0.0f, 0.0f}};
	warningLine_->SetPositions(warningPoints);
	warningLine_->SetUseWorldSpace(false);
	warningLine_->SetWidthMultiplier(0.6f);

	Log::Info("TEST LineRingTest started");
}


void ou::LineRingTest::Update() {
	const auto time = Time::GetTime();

	// 発射の向きを回しながら、壁で 4 回反射する予測線を作り直す
	const auto angle = time * 0.4f;
	const Vector3 direction = {std::cos(angle), std::sin(angle), 0.0f};
	const auto points = TraceReflection({-5.0f, -3.0f, 0.0f}, direction, 40.0f, 4);
	predictionLine_->SetPositions(points);

	// 予兆線は回りながら点滅する
	warningLine_->GetTransform()->SetLocalRotation(Quaternion::Euler({0.0f, 0.0f, time * 0.5f}));
	const auto alpha = 0.4f + 0.4f * std::sin(time * 8.0f);
	warningLine_->SetStartColor({1.0f, 0.2f, 0.2f, alpha});
	warningLine_->SetEndColor({1.0f, 0.2f, 0.2f, 0.0f});

	logTimer_ += Time::GetDeltaTime();
	if (logTimer_ >= 1.0f) {
		logTimer_ = 0.0f;
		Log::Info("TEST prediction points={}", predictionLine_->GetPositionCount());
	}
}
