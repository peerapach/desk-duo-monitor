#pragma once
#include "core.hpp"
#include "lvgl.h"
namespace deskduo {
using StockRangeSave = bool (*)(StockChartRange);
void stock_view_reset();
void stock_view_create(lv_obj_t* parent, Language language, StockRangeSave save);
void stock_view_update(const Snapshot& snapshot, uint64_t nowMs, Language language);
}
