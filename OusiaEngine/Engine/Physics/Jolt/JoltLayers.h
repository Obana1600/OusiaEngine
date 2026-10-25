#pragma once

// clang-format off
#include <Jolt/Jolt.h> // Jolt.h を先に置く
#include <Jolt/Physics/Collision/BroadPhase/BroadPhaseLayer.h>
#include <Jolt/Physics/Collision/ObjectLayer.h>
// clang-format on


namespace ou::JoltLayers {

// オブジェクトのレイヤー
namespace Object {
constexpr JPH::ObjectLayer NonMoving = 0;
constexpr JPH::ObjectLayer Moving = 1;
} // namespace Object

// ブロードフェーズのレイヤー
namespace BroadPhase {
constexpr JPH::BroadPhaseLayer NonMoving(0);
constexpr JPH::BroadPhaseLayer Moving(1);
constexpr JPH::uint Count = 2;
} // namespace BroadPhase


// オブジェクトのレイヤー -> ブロードフェーズのレイヤー
class BroadPhaseLayerInterface final: public JPH::BroadPhaseLayerInterface {
public: // メンバ関数

	JPH::uint GetNumBroadPhaseLayers() const override;
	JPH::BroadPhaseLayer GetBroadPhaseLayer(JPH::ObjectLayer layer) const override;
};

// オブジェクト + ブロードフェーズの木の組み合わせ
class ObjectVsBroadPhaseLayerFilter final: public JPH::ObjectVsBroadPhaseLayerFilter {
public: // メンバ関数

	bool ShouldCollide(JPH::ObjectLayer layer, JPH::BroadPhaseLayer broadPhaseLayer) const override;
};

// オブジェクト + オブジェクトの組み合わせ
class ObjectLayerPairFilter final: public JPH::ObjectLayerPairFilter {
public: // メンバ関数

	bool ShouldCollide(JPH::ObjectLayer a, JPH::ObjectLayer b) const override;
};

} // namespace ou::JoltLayers
