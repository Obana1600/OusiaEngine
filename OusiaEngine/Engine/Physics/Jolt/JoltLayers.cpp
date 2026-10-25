#include "JoltLayers.h"

using namespace ou;


JPH::uint ou::JoltLayers::BroadPhaseLayerInterface::GetNumBroadPhaseLayers() const {
	return BroadPhase::Count;
}


JPH::BroadPhaseLayer ou::JoltLayers::BroadPhaseLayerInterface::GetBroadPhaseLayer(JPH::ObjectLayer layer) const {
	return layer == Object::NonMoving ? BroadPhase::NonMoving
									  : BroadPhase::Moving;
}


bool ou::JoltLayers::ObjectVsBroadPhaseLayerFilter::ShouldCollide(JPH::ObjectLayer layer, JPH::BroadPhaseLayer broadPhaseLayer) const {
	if (layer == Object::NonMoving) { return broadPhaseLayer == BroadPhase::Moving; }
	return true;
}


bool ou::JoltLayers::ObjectLayerPairFilter::ShouldCollide(JPH::ObjectLayer a, JPH::ObjectLayer b) const {
	return a == Object::Moving || b == Object::Moving;
}
