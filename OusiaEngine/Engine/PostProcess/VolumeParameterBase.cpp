#include "VolumeParameterBase.h"

using namespace ou;


void ou::VolumeParameterBase::SetOverrideState(bool overrideState) {
	overrideState_ = overrideState;
}


bool ou::VolumeParameterBase::GetOverrideState() const {
	return overrideState_;
}


ou::VolumeParameterBase::VolumeParameterBase(bool overrideState)
	: overrideState_(overrideState) {}
