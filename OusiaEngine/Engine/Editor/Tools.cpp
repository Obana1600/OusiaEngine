#include "Tools.h"

using namespace ou;


void ou::Tools::SetCurrent(Tool tool) {
	sCurrent_ = tool;
}


Tool ou::Tools::GetCurrent() {
	return sCurrent_;
}


void ou::Tools::SetPivotRotation(PivotRotation pivotRotation) {
	sPivotRotation_ = pivotRotation;
}


PivotRotation ou::Tools::GetPivotRotation() {
	return sPivotRotation_;
}
