#include "AudioListener.h"
#include "Audio/Audio.h"

using namespace ou;


void ou::AudioListener::SetVolume(float volume) {
	Audio::SetVolume(volume);
}


float ou::AudioListener::GetVolume() {
	return Audio::GetVolume();
}
