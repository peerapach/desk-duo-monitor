#pragma once
#define GPIO_NUM_0 0
extern int buttonLevel;
inline int gpio_get_level(int) { return buttonLevel; }
