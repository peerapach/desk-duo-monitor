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
// The caller serializes writers; readers never perform NVS I/O.
class DisplayPreference {
    // One scalar keeps the complete display preference coherent.
    std::atomic<uint16_t> settings_{100 << 2};
public:
    DisplaySettings settings() const {
        const auto value = settings_.load();
        return {static_cast<CodexUsageMode>(value & 1), static_cast<Language>((value >> 1) & 1), static_cast<uint8_t>(value >> 2)};
    }
    CodexUsageMode get() const { return settings().mode; }
    Language language() const { return settings().language; }
    void initialize(CodexUsageMode mode, Language language = Language::Thai, uint8_t brightnessPercent = 100) {
        settings_.store(static_cast<uint16_t>(brightnessPercent << 2) | (static_cast<uint8_t>(language) << 1) | static_cast<uint8_t>(mode));
    }
    template<typename Persist> int patch(const std::string& json, Persist persist) {
        auto parsed = parseDisplayPreference(json);
        if (!parsed.value) return 400;
        auto next = settings();
        if (parsed.value->mode) next.mode = *parsed.value->mode;
        if (parsed.value->language) next.language = *parsed.value->language;
        if (parsed.value->brightnessPercent) next.brightnessPercent = *parsed.value->brightnessPercent;
        if (!persist(next)) return 503;
        initialize(next.mode, next.language, next.brightnessPercent);
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
