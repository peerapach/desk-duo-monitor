#pragma once
#include "core.hpp"
#include <ctime>
namespace deskduo {
enum class BrightnessPhase { Manual, Day, Night, ClockWait };
struct BrightnessPlan { uint8_t percent; BrightnessPhase phase; };
// Uses the process timezone and the existing clock-ready epoch threshold.
// Pure evaluation: the saved base and persistence are never changed.
BrightnessPlan brightness_plan(const DisplaySettings& settings, std::time_t epoch);
const char* brightness_phase_name(BrightnessPhase phase);
std::string brightness_time_string(uint16_t minute);
}
