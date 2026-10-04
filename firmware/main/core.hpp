#pragma once

#include <array>
#include <cstddef>
#include <cstdint>
#include <map>
#include <mutex>
#include <optional>
#include <string>
#include <utility>

namespace deskduo {

template <typename T>
struct ParseResult {
    std::optional<T> value;
    std::string error;  // Empty on success; otherwise a coarse, non-sensitive code.
};

struct UsageWindow {
    double usedPercent = 0.0;
    std::optional<double> windowMinutes;  // Positive finite minutes; fractional values are preserved.
    std::optional<int64_t> resetsAt;  // UTC epoch seconds.
};

struct Usage {
    std::string provider;  // "claude" or "codex".
    int64_t updatedAt = 0;  // UTC epoch seconds from the source event.
    std::optional<UsageWindow> primary;
    std::optional<UsageWindow> secondary;
};

struct Stock {
    std::string symbol;
    std::string label;
    std::optional<double> price;
    std::optional<double> changePercent;
    std::string currency;
    std::optional<int64_t> quotedAt;  // UTC epoch seconds.
};

struct Stocks {
    int64_t generatedAt = 0;  // UTC epoch seconds.
    std::array<Stock, 5> items{};
    std::size_t count = 0;
};

struct Weather {
    std::optional<double> temperature;
    std::optional<double> humidity;
    std::optional<int> weatherCode;
    std::optional<int64_t> updatedAt;  // UTC epoch seconds; absent for unknown local time zone.
    std::optional<int32_t> utcOffsetSeconds;
    std::array<std::optional<int64_t>, 4> hourlyTimes{};
    std::array<std::optional<double>, 4> hourlyTemperatures{};
    std::array<std::optional<double>, 4> hourlyRainProbability{};
    std::size_t hourlyCount = 0;
};

// The URL, headers and refresh interval define one independently fetched source.
// `label` and `fields` are used by weather; the stock source leaves them empty.
struct SourceConfig {
    std::string url;
    std::map<std::string, std::string> headers;
    uint32_t refreshSeconds = 0;
    std::string label;
    std::map<std::string, std::string> fields;
};

struct TouchCalibration {
    bool swapAxes = false;
    bool invertX = false;
    bool invertY = false;
    int minX = 0;
    int maxX = 4095;
    int minY = 0;
    int maxY = 4095;
};

struct Config {
    std::string ssid;
    std::string wifiPassword;
    std::string usageToken;
    std::string setupPassword;  // Internal NVS value; never accepted by public patches.
    SourceConfig weather;
    SourceConfig stocks;
    uint32_t autoRotateSeconds = 20;
    TouchCalibration touch;
};

template <typename T>
struct SourceSnapshot {
    std::optional<T> data;
    uint64_t receivedAtMs = 0;  // Monotonic receipt time; meaningful when data is present.
    bool unknown = true;
    bool stale = false;
};

struct Snapshot {
    SourceSnapshot<Usage> claude;
    SourceSnapshot<Usage> codex;
    SourceSnapshot<Stocks> stocks;
    SourceSnapshot<Weather> weather;
    std::string weatherLabel;
    uint32_t autoRotateSeconds = 20;
};

ParseResult<Usage> parseUsage(const std::string& json);
ParseResult<Stocks> parseStocks(const std::string& json);
ParseResult<Weather> parseWeather(const std::string& json, const SourceConfig& source);

Config defaultConfig();
ParseResult<Config> applyConfigPatch(const Config& current, const std::string& json);  // Public request limit: 8 KiB.
ParseResult<Config> parseStoredConfig(const std::string& json);  // Internal JSON limit: 32 KiB.
std::string serializeConfig(const Config& config);  // Internal persistence only; empty for invalid/oversize config.
std::string redactedConfig(const Config& config);  // Public authenticated settings representation.

class Model {
public:
    void applyConfig(const Config& config);
    void acceptUsage(const Usage& usage, uint64_t nowMs);
    void acceptStocks(const Stocks& stocks, uint64_t nowMs, uint64_t generation);
    void acceptWeather(const Weather& weather, uint64_t nowMs, uint64_t generation);
    void markFailed(const std::string& source, uint64_t generation);
    Snapshot snapshot(uint64_t nowMs) const;
    uint64_t generation() const;

private:
    mutable std::mutex mutex_;
    Config config_;
    uint64_t generation_ = 0;
    SourceSnapshot<Usage> claude_;
    SourceSnapshot<Usage> codex_;
    SourceSnapshot<Stocks> stocks_;
    SourceSnapshot<Weather> weather_;
};

bool redirectHeadersAllowed(const std::string& from, const std::string& to);
bool redirectTargetAllowed(const std::string& from, const std::string& to);
std::pair<int, int> mapTouch(int rawX, int rawY, const TouchCalibration& calibration);

}  // namespace deskduo
