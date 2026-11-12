#include "Keyframe.h"


ou::Keyframe::Keyframe()
	: time(0.0f),
	  value(0.0f),
	  inTangent(0.0f),
	  outTangent(0.0f),
	  inWeight(0.0f),
	  outWeight(0.0f),
	  weightedMode(WeightedMode::None) {}


ou::Keyframe::Keyframe(float time, float value)
	: time(time),
	  value(value),
	  inTangent(0.0f),
	  outTangent(0.0f),
	  inWeight(0.0f),
	  outWeight(0.0f),
	  weightedMode(WeightedMode::None) {}


ou::Keyframe::Keyframe(float time, float value, float inTangent, float outTangent)
	: time(time),
	  value(value),
	  inTangent(inTangent),
	  outTangent(outTangent),
	  inWeight(0.0f),
	  outWeight(0.0f),
	  weightedMode(WeightedMode::None) {}


ou::Keyframe::Keyframe(float time, float value, float inTangent, float outTangent, float inWeight, float outWeight)
	: time(time),
	  value(value),
	  inTangent(inTangent),
	  outTangent(outTangent),
	  inWeight(inWeight),
	  outWeight(outWeight),
	  weightedMode(WeightedMode::Both) {}
