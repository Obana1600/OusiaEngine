#pragma once

// clang-format off
#include <Jolt/Jolt.h> // Jolt.h を先に置く
#include <Jolt/Physics/Body/BodyFilter.h>
// clang-format on


namespace ou::JoltQueryFilters {

class IgnoreSensorFilter final: public JPH::BodyFilter {
public: // メンバ関数

	bool ShouldCollideLocked(const JPH::Body &body) const override;
};

const JPH::BodyFilter &SelectBodyFilter(bool hitsSensors);

} // namespace ou::JoltQueryFilters
