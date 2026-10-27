#include "TrajectoryPreview.h"
#include "Motion2D.h"
#include "Player.h"
#include "Render/LineRenderer.h"
#include "Scene/GameObject.h"
#include "Scene/Scene.h"
#include "Tuning.h"

#include <vector>

using namespace ou;


namespace {

// 予測線の反射回数に上限はない（時間と速さで止まる）
constexpr int kUnlimitedBounces = 1000;

} // namespace



void ou::TrajectoryPreview::Start() {
	auto *object = GetGameObject()->GetScene()->CreateGameObject("Trajectory Line");
	line_ = object->AddComponent<LineRenderer>();
	line_->SetStartWidth(0.25f);
	line_->SetEndWidth(0.05f);
	line_->SetStartColor({1.0f, 1.0f, 1.0f, 0.9f});
	line_->SetEndColor({0.3f, 0.8f, 1.0f, 0.0f});
	object->SetActive(false);
}


void ou::TrajectoryPreview::LateUpdate() {
	const auto *player = Player::GetInstance();
	const auto isCharging = player && player->GetPull().IsCharging();
	line_->GetGameObject()->SetActive(isCharging);
	if (!isCharging) { return; }

	// 今の位置から、今の溜めで発射したとして、低速帯に落ちるまでをなぞる
	MotionState state = player->GetMotion();
	state.velocity = player->ComputeLaunchVelocity();
	const auto points = Motion2D::Trace(state, Player::MakeMotionParams(), Tuning::kPreviewMaxTime, Tuning::kPreviewStep,
										kUnlimitedBounces, Tuning::kDamageSpeed);

	std::vector<Vector3> positions;
	positions.reserve(points.size());
	for (const auto &point : points) { positions.push_back({point.x, point.y, 0.2f}); }
	line_->SetPositions(positions);
}
