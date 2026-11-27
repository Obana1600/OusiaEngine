#include "Exposure.h"

using namespace ou;


ou::Exposure::Exposure() {
	AddParameter("mode", mode_);
	AddParameter("fixedExposure", fixedExposure_);
	AddParameter("compensation", compensation_);
	AddParameter("limitMin", limitMin_);
	AddParameter("limitMax", limitMax_);
	AddParameter("adaptationMode", adaptationMode_);
	AddParameter("adaptationSpeedDarkToLight", adaptationSpeedDarkToLight_);
	AddParameter("adaptationSpeedLightToDark", adaptationSpeedLightToDark_);
	AddParameter("histogramPercentages", histogramPercentages_);
}


bool ou::Exposure::IsActive() const {
	return mode_.GetOverrideState();
}
