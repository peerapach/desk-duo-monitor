#pragma once
#include "core.hpp"
#include "weather_diagnostics.hpp"
#include <atomic>
namespace deskduo {
// Verify the actual serialized result restores every intended field, including
// secrets. A nonempty cJSON print alone does not prove all allocations succeeded.
inline bool completeSerialization(const Config& expected, const std::string& json) {
    auto parsed = parseStoredConfig(json);
    if (!parsed.value) return false;
    const auto& actual = *parsed.value;
    auto sameSource = [](const SourceConfig& a, const SourceConfig& b) {
        return a.url == b.url && a.headers == b.headers && a.refreshSeconds == b.refreshSeconds && a.label == b.label && a.fields == b.fields;
    };
    const auto& a = actual.touch; const auto& b = expected.touch;
    return actual.ssid == expected.ssid && actual.wifiPassword == expected.wifiPassword &&
        actual.setupPassword == expected.setupPassword && actual.usageToken == expected.usageToken &&
        actual.autoRotateSeconds == expected.autoRotateSeconds &&
        sameSource(actual.weather, expected.weather) && sameSource(actual.stocks, expected.stocks) &&
        a.swapAxes == b.swapAxes && a.invertX == b.invertX && a.invertY == b.invertY &&
        a.minX == b.minX && a.maxX == b.maxX && a.minY == b.minY && a.maxY == b.maxY;
}
// One unsigned scalar is authoritative both in NVS and for concurrent readers.
inline uint32_t encodeDisplaySettings(const DisplaySettings& value) {
    return static_cast<uint32_t>(value.mode) | (static_cast<uint32_t>(value.language) << 1) |
        (static_cast<uint32_t>(value.brightnessPercent) << 2) | (static_cast<uint32_t>(value.brightnessAuto) << 9) |
        (static_cast<uint32_t>(value.brightnessDayStart) << 10) | (static_cast<uint32_t>(value.brightnessNightStart) << 21);
}
inline DisplaySettings decodeDisplaySettings(uint32_t value) {
    return {static_cast<CodexUsageMode>(value & 1u), static_cast<Language>((value >> 1) & 1u),
        static_cast<uint8_t>((value >> 2) & 127u), bool((value >> 9) & 1u),
        static_cast<uint16_t>((value >> 10) & 2047u), static_cast<uint16_t>((value >> 21) & 2047u)};
}
inline bool validDisplaySettings(const DisplaySettings& value) {
    return value.brightnessPercent >= 5 && value.brightnessPercent <= 100 &&
        value.brightnessDayStart < 1440 && value.brightnessNightStart < 1440 && value.brightnessDayStart != value.brightnessNightStart;
}
// The caller serializes writers; readers never perform NVS I/O.
class DisplayPreference {
    // One scalar keeps the complete display preference coherent.
    std::atomic<uint32_t> settings_{encodeDisplaySettings(DisplaySettings{})};
public:
    DisplaySettings settings() const { return decodeDisplaySettings(settings_.load()); }
    CodexUsageMode get() const { return settings().mode; }
    Language language() const { return settings().language; }
    void initialize(CodexUsageMode mode, Language language = Language::Thai, uint8_t brightnessPercent = 100,
                    bool brightnessAuto = false, uint16_t dayStart = 360, uint16_t nightStart = 1080) {
        settings_.store(encodeDisplaySettings({mode, language, brightnessPercent, brightnessAuto, dayStart, nightStart}));
    }
    void initialize(const DisplaySettings& settings) { settings_.store(encodeDisplaySettings(settings)); }
    template<typename Persist> int patch(const std::string& json, Persist persist) {
        auto parsed = parseDisplayPreference(json);
        if (!parsed.value) return 400;
        auto next = settings();
        if (parsed.value->mode) next.mode = *parsed.value->mode;
        if (parsed.value->language) next.language = *parsed.value->language;
        if (parsed.value->brightnessPercent) next.brightnessPercent = *parsed.value->brightnessPercent;
        if (parsed.value->brightnessAuto) next.brightnessAuto = *parsed.value->brightnessAuto;
        if (parsed.value->brightnessDayStart) next.brightnessDayStart = *parsed.value->brightnessDayStart;
        if (parsed.value->brightnessNightStart) next.brightnessNightStart = *parsed.value->brightnessNightStart;
        if (!validDisplaySettings(next)) return 400;
        if (!persist(next)) return 503;
        initialize(next);
        return 204;
    }
};
DisplaySettings runtime_display_settings();
Language runtime_language();
CodexUsageMode runtime_display_mode();
int runtime_display_patch(const std::string& json);
struct RequestConfig { Config config; uint64_t generation; };
void runtime_init();
RequestConfig runtime_config();
Snapshot runtime_snapshot();
WeatherView runtime_weather_view();
int runtime_patch(const std::string& json);
void runtime_usage(const Usage& usage);
void runtime_weather(const ParseResult<Weather>& value, uint64_t generation);
void runtime_stocks(const ParseResult<Stocks>& value, uint64_t generation);
uint64_t now_ms();
std::string ap_name();
}
