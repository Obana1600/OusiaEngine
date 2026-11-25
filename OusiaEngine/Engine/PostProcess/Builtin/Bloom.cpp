#include "Bloom.h"

using namespace ou;


ou::Bloom::Bloom() {
	AddParameter("threshold", threshold_);
	AddParameter("intensity", intensity_);
	AddParameter("scatter", scatter_);
	AddParameter("clamp", clamp_);
	AddParameter("tint", tint_);
}


bool ou::Bloom::IsActive() const {
	return intensity_.GetValue() > 0.0f;
}
