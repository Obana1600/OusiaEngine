#include "JoltQueryFilters.h"

#include <Jolt/Physics/Body/Body.h>

using namespace ou;


bool ou::JoltQueryFilters::IgnoreSensorFilter::ShouldCollideLocked(const JPH::Body &body) const {
	return !body.IsSensor();
}


const JPH::BodyFilter &ou::JoltQueryFilters::SelectBodyFilter(bool hitsSensors) {
	static const JPH::BodyFilter sAllFilter;
	static const IgnoreSensorFilter sIgnoreSensorFilter;
	return hitsSensors ? sAllFilter : sIgnoreSensorFilter;
}
