#include "Screen.h"

using namespace ou;


namespace {

// グローバルなスクリーンサイズ
uint32_t sWidth = 0;
uint32_t sHeight = 0;

} // namespace



uint32_t ou::Screen::GetWidth() {
	return sWidth;
}


uint32_t ou::Screen::GetHeight() {
	return sHeight;
}


void ou::Screen::SetSize(uint32_t width, uint32_t height) {
	sWidth = width;
	sHeight = height;
}
