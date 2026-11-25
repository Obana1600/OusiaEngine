#include "Tonemapping.h"

using namespace ou;


ou::Tonemapping::Tonemapping() {
	AddParameter("mode", mode_);
}


bool ou::Tonemapping::IsActive() const {
	return mode_.GetValue() != TonemappingMode::None;
}
