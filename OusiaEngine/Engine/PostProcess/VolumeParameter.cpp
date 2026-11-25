#include "VolumeParameter.h"
#include "Math/Mathf.h"

using namespace ou;


ou::ClampedFloatParameter::ClampedFloatParameter(float value, float min, float max, bool overrideState)
	: VolumeParameter<float>(Mathf::Clamp(value, min, max), overrideState),
	  min_(min),
	  max_(max) {}


void ou::ClampedFloatParameter::SetValue(float value) {
	value_ = Mathf::Clamp(value, min_, max_);
}


float ou::ClampedFloatParameter::GetMin() const {
	return min_;
}


float ou::ClampedFloatParameter::GetMax() const {
	return max_;
}
