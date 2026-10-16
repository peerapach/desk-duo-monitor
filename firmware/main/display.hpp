#pragma once
#include <cstdint>
#include "backlight.hpp"
namespace deskduo {
void display_init(uint8_t brightnessPercent = 100);
void display_flush(int x1, int y1, int x2, int y2, uint8_t* pixels);
}
