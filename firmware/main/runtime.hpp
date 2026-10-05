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
    std::atomic<CodexUsageMode> mode_{CodexUsageMode::Both};
public:
    CodexUsageMode get() const { return mode_.load(); }
    void initialize(CodexUsageMode mode) { mode_.store(mode); }
    template<typename Persist> int patch(const std::string& json, Persist persist) {
        auto parsed = parseDisplayPreference(json);
        if (!parsed.value) return 400;
        if (!persist(*parsed.value)) return 503;
        mode_.store(*parsed.value);
        return 204;
    }
};
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
