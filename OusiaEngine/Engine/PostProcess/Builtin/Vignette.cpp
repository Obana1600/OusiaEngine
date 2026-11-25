#include "Vignette.h"

using namespace ou;


ou::Vignette::Vignette() {
	AddParameter("color", color_);
	AddParameter("center", center_);
	AddParameter("intensity", intensity_);
	AddParameter("smoothness", smoothness_);
	AddParameter("rounded", rounded_);
}


bool ou::Vignette::IsActive() const {
	return intensity_.GetValue() > 0.0f;
}
