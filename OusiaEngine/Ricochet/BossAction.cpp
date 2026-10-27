#include "BossAction.h"
#include "Boss.h"
#include "Math/Mathf.h"
#include "Player.h"

using namespace ou;



Vector2 ou::DirectionToPlayer(const Boss &boss) {
	const auto *player = Player::GetInstance();
	if (!player) { return {-1.0f, 0.0f}; }
	const auto toPlayer = player->GetPosition() - boss.GetPosition();
	if (toPlayer.LengthSquared() < 0.0001f) { return {-1.0f, 0.0f}; }
	return toPlayer.Normalize();
}


Vector2 ou::RotateDirection(const Vector2 &direction, float angle) {
	const auto cos = Mathf::Cos(angle);
	const auto sin = Mathf::Sin(angle);
	return Vector2{direction.x * cos - direction.y * sin, direction.x * sin + direction.y * cos}.Normalize();
}
