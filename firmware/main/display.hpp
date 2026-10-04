#pragma once
#include <cstdint>
namespace deskduo {
void display_init();
void display_flush(int x1, int y1, int x2, int y2, uint8_t* pixels);
}
