#pragma once
#include "esp_err.h"
#include <cstdint>
#include <optional>
namespace deskduo {
// Only the main/UI task owns backlight initialization and later application.
// Successful repeated levels are cached; errors remain retryable.
// HTTP may read this snapshot; only successful hardware requests publish it.
std::optional<uint8_t> backlight_last_applied_percent();
esp_err_t backlight_init(uint8_t brightnessPercent);
esp_err_t display_set_brightness(uint8_t brightnessPercent);
}
