#include "brightness_policy.hpp"
#include <algorithm>
#include <cstdio>
namespace deskduo {
BrightnessPlan brightness_plan(const DisplaySettings& settings, std::time_t epoch) {
    if (!settings.brightnessAuto) return {settings.brightnessPercent, BrightnessPhase::Manual};
    std::tm local{};
    if (epoch < 1700000000 || !localtime_r(&epoch, &local)) return {settings.brightnessPercent, BrightnessPhase::ClockWait};
    const auto minute = local.tm_hour * 60 + local.tm_min;
    const auto start = settings.brightnessDayStart, end = settings.brightnessNightStart;
    const bool day = start < end ? minute >= start && minute < end : minute >= start || minute < end;
    return day ? BrightnessPlan{settings.brightnessPercent, BrightnessPhase::Day} :
        BrightnessPlan{static_cast<uint8_t>(std::max(5, (settings.brightnessPercent + 1) / 2)), BrightnessPhase::Night};
}
const char* brightness_phase_name(BrightnessPhase phase) {
    switch (phase) {
        case BrightnessPhase::Manual: return "manual";
        case BrightnessPhase::Day: return "day";
        case BrightnessPhase::Night: return "night";
        case BrightnessPhase::ClockWait: return "clock_wait";
    }
    return "clock_wait";
}
std::string brightness_time_string(uint16_t minute) {
    // uint16_t also permits 1092:15; keep the full argument domain bounded.
    char text[8]; std::snprintf(text, sizeof(text), "%02u:%02u", unsigned(minute / 60), unsigned(minute % 60));
    return text;
}
}
