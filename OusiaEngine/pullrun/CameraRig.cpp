#include "CameraRig.h"
#include "Core/Time.h"
#include "Input/Input.h"
#include "Math/Mathf.h"
#include "Math/Random.h"
#include "Player.h"
#include "Render/Camera.h"
#include "Scene/GameObject.h"
#include "Scene/Transform.h"
#include "Tuning.h"

#include <algorithm>
#include <cmath>

using namespace ou;


namespace {

constexpr float kShakeDecay = 6.0f;

} // namespace



void ou::CameraRig::Awake() {
	sInstance_ = this;
	camera_ = GetGameObject()->AddComponent<Camera>();
	camera_->SetFieldOfView(Tuning::kCameraFieldOfView);
	camera_->SetFarClipPlane(500.0f);
	viewHeight_ = ComputeFullViewHeight();
}


void ou::CameraRig::LateUpdate() {
	// スロー中やヒットストップ中も追従が遅れないよう、実時間で補間する
	const auto dt = Time::GetUnscaledDeltaTime();
	const auto *player = Player::GetInstance();

	auto targetZoom = 0.0f;
	Vector2 playerPosition;
	if (player) {
		playerPosition = player->GetPosition();
		const auto &pull = player->GetPull();
		if (pull.IsCharging()) {
			targetZoom = pull.GetChargeRatio();
			heldZoom_ = targetZoom;
		} else if (player->GetBand() == SpeedBand::High) {
			// 発射後、速さが一定以下になるまでは引いたまま
			targetZoom = heldZoom_;
		} else {
			heldZoom_ = 0.0f;
		}
	}

	const auto blend = 1.0f - std::exp(-Tuning::kCameraFollowSharpness * dt);
	zoom_ = Mathf::Lerp(zoom_, targetZoom, blend);

	// 寄りはステージの約 2/3、全体はステージ全体が収まる高さ
	const auto fullHeight = ComputeFullViewHeight();
	const auto normalHeight = fullHeight * Tuning::kCameraNormalViewRatio;
	viewHeight_ = Mathf::Lerp(normalHeight, fullHeight, zoom_);

	// 寄りのときはプレイヤーを中心に、ステージの外ができるだけ映らないようにずらす
	const auto aspect = camera_->GetAspect();
	const auto halfHeight = viewHeight_ * 0.5f;
	const auto halfWidth = halfHeight * aspect;
	const auto limitX = std::max(Tuning::kArenaHalfWidth + 1.0f - halfWidth, 0.0f);
	const auto limitY = std::max(Tuning::kArenaHalfHeight + 1.0f - halfHeight, 0.0f);
	const Vector2 followCenter = {std::clamp(playerPosition.x, -limitX, limitX), std::clamp(playerPosition.y, -limitY, limitY)};
	const auto targetCenter = Vector2::Lerp(followCenter, Vector2::Zero, zoom_);
	center_ = Vector2::Lerp(center_, targetCenter, blend);

	// 揺れ
	shake_ *= std::exp(-kShakeDecay * dt);
	const auto shakeOffset = Random::InsideUnitCircle() * shake_;
	if (shake_ < 0.02f && Input::IsGamepadConnected()) { Input::SetGamepadMotorSpeeds(0.0f, 0.0f); }

	// 見たい高さが画面に収まる距離まで下がる
	const auto distance = halfHeight / std::tan(Tuning::kCameraFieldOfView * 0.5f * Mathf::Deg2Rad);
	GetTransform()->SetLocalPosition({center_.x + shakeOffset.x, center_.y + shakeOffset.y, -distance});
}


void ou::CameraRig::OnDestroy() {
	Input::SetGamepadMotorSpeeds(0.0f, 0.0f);
	if (sInstance_ == this) { sInstance_ = nullptr; }
}


void ou::CameraRig::Shake(float strength) {
	shake_ = std::max(shake_, strength);
	const auto motor = Mathf::Clamp01(strength * 2.0f);
	Input::SetGamepadMotorSpeeds(motor, motor);
}


float ou::CameraRig::ComputeFullViewHeight() const {
	// 幅と高さのどちらで決まるかは画面の縦横比による
	const auto aspect = camera_ ? camera_->GetAspect() : 16.0f / 9.0f;
	const auto height = std::max(Tuning::kArenaHalfHeight * 2.0f, Tuning::kArenaHalfWidth * 2.0f / aspect);
	return height * Tuning::kCameraFullViewMargin;
}
