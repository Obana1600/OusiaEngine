#include "Motion2D.h"
#include "Math/Mathf.h"
#include "Tuning.h"

#include <algorithm>
#include <cmath>

using namespace ou;


namespace {

// 1 回のサブステップで進んでよい距離（半径に対する割合）
constexpr float kMaxTravelPerSubstep = 0.5f;
constexpr int kMaxSubsteps = 32;

// 壁を越えていたら押し戻して反射する。反射したら true
bool ResolveWalls(MotionState &state, const MotionParams &params, Vector2 *wallNormal) {
	const auto right = Tuning::kArenaHalfWidth - params.radius;
	const auto top = Tuning::kArenaHalfHeight - params.radius;
	auto bounced = false;

	if (state.position.x > right && state.velocity.x > 0.0f) {
		state.position.x = right;
		state.velocity.x = -state.velocity.x;
		*wallNormal = {-1.0f, 0.0f};
		bounced = true;
	} else if (state.position.x < -right && state.velocity.x < 0.0f) {
		state.position.x = -right;
		state.velocity.x = -state.velocity.x;
		*wallNormal = {1.0f, 0.0f};
		bounced = true;
	}

	if (state.position.y > top && state.velocity.y > 0.0f) {
		state.position.y = top;
		state.velocity.y = -state.velocity.y;
		*wallNormal = {0.0f, -1.0f};
		bounced = true;
	} else if (state.position.y < -top && state.velocity.y < 0.0f) {
		state.position.y = -top;
		state.velocity.y = -state.velocity.y;
		*wallNormal = {0.0f, 1.0f};
		bounced = true;
	}

	// 速さが 0 で壁の外にいるときも押し戻す
	state.position.x = std::clamp(state.position.x, -right, right);
	state.position.y = std::clamp(state.position.y, -top, top);

	if (bounced) { state.velocity *= params.restitution; }
	return bounced;
}

} // namespace



MotionStepResult ou::Motion2D::Step(MotionState &state, const MotionParams &params, float dt) {
	MotionStepResult result;
	if (dt <= 0.0f) { return result; }

	const auto travel = state.velocity.Length() * dt;
	const auto maxTravel = std::max(params.radius * kMaxTravelPerSubstep, 0.01f);
	const auto substeps = std::clamp(static_cast<int>(std::ceil(travel / maxTravel)), 1, kMaxSubsteps);
	const auto subDt = dt / static_cast<float>(substeps);
	const auto dragFactor = std::exp(-params.drag * subDt);

	for (int i = 0; i < substeps; i++) {
		// 重力で加速しても、重力がかかる速度帯からは出ないようにする
		const auto speedBefore = state.velocity.Length();
		if (params.gravity > 0.0f && speedBefore < params.gravityBelowSpeed) {
			state.velocity.y -= params.gravity * subDt;
			const auto speedAfter = state.velocity.Length();
			const auto limit = params.gravityBelowSpeed * 0.95f;
			if (speedAfter > limit) { state.velocity *= limit / speedAfter; }
		}

		state.velocity *= dragFactor;
		state.position += state.velocity * subDt;

		const auto velocityBefore = state.velocity;
		if (ResolveWalls(state, params, &result.lastWallNormal)) {
			result.bounceCount++;
			result.wallImpulse += state.velocity - velocityBefore;
		}
	}
	return result;
}


std::vector<Vector2> ou::Motion2D::Trace(MotionState state, const MotionParams &params, float duration, float stepTime,
										 int maxBounces, float stopBelowSpeed) {
	std::vector<Vector2> points = {state.position};
	auto bounces = 0;
	for (auto time = 0.0f; time < duration; time += stepTime) {
		bounces += Step(state, params, stepTime).bounceCount;
		points.push_back(state.position);
		if (bounces >= maxBounces) { break; }
		if (state.velocity.Length() < stopBelowSpeed) { break; }
	}
	return points;
}


Vector2 ou::Motion2D::Reflect(const Vector2 &velocity, const Vector2 &normal) {
	const auto dot = Vector2::Dot(velocity, normal);
	// すでに離れる向きなら反射しない
	if (dot >= 0.0f) { return velocity; }
	return velocity - normal * (2.0f * dot);
}
