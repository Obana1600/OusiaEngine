#include "Arena.h"
#include "GameAssets.h"
#include "Tuning.h"

using namespace ou;


namespace {

constexpr float kWallThickness = 1.0f;
constexpr float kWallDepth = 2.0f;
const Color kWallColor = {0.35f, 0.38f, 0.45f, 1.0f};
const Color kFloorColor = {0.08f, 0.09f, 0.12f, 1.0f};

} // namespace



void ou::CreateArena(Scene &scene) {
	const auto &cube = GameAssets::GetInstance()->GetCube();
	const auto halfWidth = Tuning::kArenaHalfWidth;
	const auto halfHeight = Tuning::kArenaHalfHeight;
	const auto outerWidth = (halfWidth + kWallThickness) * 2.0f;
	const auto offsetX = halfWidth + kWallThickness * 0.5f;
	const auto offsetY = halfHeight + kWallThickness * 0.5f;

	// 壁の内側の面がちょうどアリーナの境界になる
	CreateMeshObject(scene, "Wall Top", cube, {0.0f, offsetY, 0.0f}, {outerWidth, kWallThickness, kWallDepth}, kWallColor, false);
	CreateMeshObject(scene, "Wall Bottom", cube, {0.0f, -offsetY, 0.0f}, {outerWidth, kWallThickness, kWallDepth}, kWallColor, false);
	CreateMeshObject(scene, "Wall Left", cube, {-offsetX, 0.0f, 0.0f}, {kWallThickness, halfHeight * 2.0f, kWallDepth}, kWallColor, false);
	CreateMeshObject(scene, "Wall Right", cube, {offsetX, 0.0f, 0.0f}, {kWallThickness, halfHeight * 2.0f, kWallDepth}, kWallColor, false);

	// 奥の床（xy 平面の背景）
	CreateMeshObject(scene, "Floor", cube, {0.0f, 0.0f, kWallDepth * 0.5f + 0.05f}, {halfWidth * 2.0f, halfHeight * 2.0f, 0.1f}, kFloorColor, true);
}
