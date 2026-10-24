#pragma once
#include "core.hpp"
#include <string_view>
namespace deskduo {
enum class WeatherPhase : uint8_t { None, Gate, Dns, Tcp, Tls, Http, Parse, Complete };
enum class WeatherOutcome : uint8_t {
    NotAttempted, Running, ClockWait, InvalidTarget, DnsAllocation, DnsQueue,
    DnsFailure, Deadline, TransportAllocation, HttpAllocation, TcpFailure,
    TlsFailure, HttpHeaders, HttpPerform, HttpStatus, WireLimit, BodyLimit,
    RedirectHeader, RedirectRejected, ParseFailure, Accepted, Obsolete
};
enum class WeatherParserReason : uint8_t {
    None, Other, Size, Json, Duplicate, Depth, Shape, UpstreamError, Mapping,
    Temperature, Humidity, WeatherCode, UtcOffset, UpdatedAt, Hourly,
    HourlyTime, HourlyTemperature, RainProbability, NoReadings, IsDay, HourlyWeatherCode, HourlyIsDay
};
struct WeatherHeap {
    std::optional<uint32_t> freeBytes, largestBlock;
};
// Fixed storage only: never retain URL, headers, payload or SDK/parser text.
struct WeatherAttempt {
    bool attempted = false, clockReady = false;
    uint64_t generation = 0, startedMs = 0, elapsedMs = 0, nextDueMs = 0;
    WeatherPhase phase = WeatherPhase::None;
    WeatherOutcome outcome = WeatherOutcome::NotAttempted;
    WeatherParserReason parserReason = WeatherParserReason::None;
    std::optional<int> httpResult, httpStatus, espError, tlsCode, verifyFlags, socketErrno;
    WeatherHeap beforeHttp, beforeTls, failureHeap;
    uint32_t wireBytes = 0, bodyBytes = 0, redirects = 0;
    void fail(WeatherOutcome result, WeatherPhase at) {
        if (outcome == WeatherOutcome::Running) { outcome = result; phase = at; }
    }
};
class WeatherDiagnostics {
    mutable std::mutex mutex_;
    WeatherAttempt latest_;
public:
    WeatherAttempt begin(uint64_t generation, uint64_t now, bool clockReady);
    void finish(WeatherAttempt record, uint64_t now, uint64_t nextDue);
    WeatherAttempt snapshot() const;
};
// Config generation/configured and model data captured under the runtime config lock.
struct WeatherView {
    uint64_t generation = 0;
    bool configured = false;
    SourceSnapshot<Weather> weather;
};
WeatherParserReason weatherParserReason(std::string_view reason);
std::string weatherDiagnosticsJson(const WeatherAttempt& attempt, const WeatherView& current,
                                  bool online, int64_t epochSeconds, uint64_t now);
}
