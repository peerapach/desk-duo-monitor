#pragma once

#include "core.hpp"
#include "lvgl.h"

namespace deskduo {
// Called by the existing UI task; reset before deleting the content viewport.
void weather_view_reset();
void weather_view_create(lv_obj_t* parent, Language language);
void weather_view_update(const Snapshot& snapshot, uint64_t nowMs, Language language);
}
