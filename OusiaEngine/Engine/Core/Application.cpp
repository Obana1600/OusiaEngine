#include "Application.h"


namespace {

bool sIsQuitRequested = false;

} // namespace



void ou::Application::Quit() {
	sIsQuitRequested = true;
}


bool ou::Application::IsQuitRequested() {
	return sIsQuitRequested;
}
