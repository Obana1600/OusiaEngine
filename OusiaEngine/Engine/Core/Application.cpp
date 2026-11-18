#include "Application.h"
#ifdef WITH_EDITOR
#include "Editor/EditorApplication.h"
#endif


namespace {

bool sIsQuitRequested = false;

} // namespace



bool ou::Application::IsPlaying() {
#ifdef WITH_EDITOR
	return EditorApplication::IsPlaying();
#else
	return true;
#endif
}


bool ou::Application::IsPaused() {
#ifdef WITH_EDITOR
	return EditorApplication::IsGameFrameSkipped();
#else
	return false;
#endif
}


void ou::Application::Quit() {
	sIsQuitRequested = true;
}


bool ou::Application::IsQuitRequested() {
	return sIsQuitRequested;
}
