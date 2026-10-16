#pragma once

#include <cstdint>


namespace ou::Screen {

uint32_t GetWidth();
uint32_t GetHeight();

void SetSize(uint32_t width, uint32_t height);

} // namespace ou::Screen
