#include "Volume.h"
#include "Math/Mathf.h"
#include "PostProcess/VolumeManager.h"
#include "PostProcess/VolumeProfile.h"
#include "Serialize/Serializer.h"

#include <utility>

using namespace ou;


void ou::Volume::OnEnable() {
	VolumeManager::Register(this);
}


void ou::Volume::OnDisable() {
	VolumeManager::Unregister(this);
}


void ou::Volume::Serialize(Serializer &serializer) {
	Behavior::Serialize(serializer);

	serializer.Field("isGlobal", isGlobal_);
	serializer.Field("priority", priority_);
	serializer.Field("weight", weight_);
	serializer.Asset("sharedProfile", sharedProfile_);

	if (serializer.IsReading()) { SetWeight(weight_); }
}


void ou::Volume::SetIsGlobal(bool isGlobal) {
	isGlobal_ = isGlobal;
}


bool ou::Volume::GetIsGlobal() const {
	return isGlobal_;
}


void ou::Volume::SetPriority(float priority) {
	priority_ = priority;
}


float ou::Volume::GetPriority() const {
	return priority_;
}


void ou::Volume::SetWeight(float weight) {
	weight_ = Mathf::Clamp01(weight);
}


float ou::Volume::GetWeight() const {
	return weight_;
}


void ou::Volume::SetSharedProfile(std::shared_ptr<VolumeProfile> sharedProfile) {
	sharedProfile_ = std::move(sharedProfile);
}


const std::shared_ptr<VolumeProfile> &ou::Volume::GetSharedProfile() const {
	return sharedProfile_;
}
