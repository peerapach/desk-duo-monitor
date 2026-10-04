#pragma once
#include "lvgl.h"
namespace deskduo {
// Provider 0 is Claude, 1 is Codex. Invalid providers return nullptr.
const lv_image_dsc_t* character_image(unsigned provider, bool mini = false);
}
