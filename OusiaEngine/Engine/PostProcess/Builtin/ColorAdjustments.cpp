#include "ColorAdjustments.h"

using namespace ou;


ou::ColorAdjustments::ColorAdjustments() {
	AddParameter("postExposure", postExposure_);
	AddParameter("contrast", contrast_);
	AddParameter("colorFilter", colorFilter_);
	AddParameter("hueShift", hueShift_);
	AddParameter("saturation", saturation_);
}


bool ou::ColorAdjustments::IsActive() const {
	return postExposure_.GetValue() != 0.0f ||
		   contrast_.GetValue() != 0.0f ||
		   colorFilter_.GetValue() != Color::White ||
		   hueShift_.GetValue() != 0.0f ||
		   saturation_.GetValue() != 0.0f;
}
