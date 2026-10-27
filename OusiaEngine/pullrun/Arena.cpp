#include "Arena.h"
#include "GameAssets.h"
#include "Render/Asset/Material.h"
#include "Render/MeshRenderer.h"
#include "Tuning.h"

using namespace ou;


namespace {

constexpr float kWallThickness = 1.0f;
constexpr float kWallDepth = 2.0f;
const Color kWallColor = {0.35f, 0.38f, 0.45f, 1.0f};
// 床はライトを受けて、点光源の光だまりが映るようにする
const Color kFloorColor = {0.3f, 0.32f, 0.38f, 0.55f};

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

	// 奥の床（xy 平面の背景）。半透明にして、奥の天球を透かす
	auto *floor = CreateMeshObject(scene, "Floor", cube, {0.0f, 0.0f, kWallDepth * 0.5f + 0.05f}, {halfWidth * 2.0f, halfHeight * 2.0f, 0.1f}, kFloorColor, false);
	const auto &material = floor->GetSharedMaterial();
	material->SetRenderQueue(Shader::RenderQueue::Transparent);
	Shader::RenderStateOverride override{};
	override.srcBlend = Graphics::Blend::SRC_ALPHA;
	override.dstBlend = Graphics::Blend::INV_SRC_ALPHA;
	override.blendOp = Graphics::BlendOp::ADD;
	override.zWrite = false;
	material->SetRenderStateOverride(override);
}
