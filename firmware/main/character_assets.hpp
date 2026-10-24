#pragma once
#include "lvgl.h"
#include <optional>
namespace deskduo {
// Provider 0 is Claude, 1 is Codex. Invalid providers return nullptr.
const lv_image_dsc_t* character_image(unsigned provider, bool mini = false);
const lv_image_dsc_t* navigation_image(unsigned tab, bool active);
const lv_image_dsc_t* weather_image(std::optional<int> code, std::optional<bool> day, bool mini=false);
}
