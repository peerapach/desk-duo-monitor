#include "weather_diagnostics.hpp"
#include "cJSON.h"
#include <cstdlib>
namespace deskduo {
WeatherAttempt WeatherDiagnostics::begin(uint64_t generation, uint64_t now, bool clockReady) {
    WeatherAttempt record;
    record.attempted = true; record.clockReady = clockReady;
    record.generation = generation; record.startedMs = now;
    record.outcome = WeatherOutcome::Running; record.phase = WeatherPhase::Gate;
    const std::lock_guard<std::mutex> lock(mutex_); latest_ = record; return record;
}
void WeatherDiagnostics::finish(WeatherAttempt record, uint64_t now, uint64_t nextDue) {
    record.elapsedMs = now >= record.startedMs ? now - record.startedMs : 0;
    record.nextDueMs = nextDue;
    const std::lock_guard<std::mutex> lock(mutex_); latest_ = record;
}
WeatherAttempt WeatherDiagnostics::snapshot() const {
    const std::lock_guard<std::mutex> lock(mutex_); return latest_;
}
namespace {
constexpr const char* parserNames[] = {"none", "other", "size", "json", "duplicate", "depth", "shape",
    "upstream_error", "mapping", "temperature", "humidity", "weather_code", "utc_offset", "updated_at",
    "hourly", "hourly_time", "hourly_temperature", "rain_probability", "no_readings"};
template<class T> void number(cJSON* object, const char* name, const std::optional<T>& value) {
    if (value) cJSON_AddNumberToObject(object, name, *value); else cJSON_AddNullToObject(object, name);
}
void heap(cJSON* object, const char* name, const WeatherHeap& value) {
    auto* child = cJSON_AddObjectToObject(object, name);
    number(child, "freeBytes", value.freeBytes); number(child, "largestBlock", value.largestBlock);
}
}
WeatherParserReason weatherParserReason(std::string_view reason) {
    for (unsigned i = 0; i < sizeof(parserNames)/sizeof(*parserNames); ++i)
        if (reason == parserNames[i]) return static_cast<WeatherParserReason>(i);
    return WeatherParserReason::Other;
}
std::string weatherDiagnosticsJson(const WeatherAttempt& a, const WeatherView& current,
                                  bool online, int64_t epochSeconds, uint64_t now) {
    constexpr const char* phases[] = {"none", "gate", "dns", "tcp", "tls", "http", "parse", "complete"};
    constexpr const char* outcomes[] = {"not_attempted", "running", "clock_wait", "invalid_target", "dns_allocation",
        "dns_queue", "dns_failure", "deadline", "transport_allocation", "http_allocation", "tcp_failure",
        "tls_failure", "http_headers", "http_perform", "http_status", "wire_limit", "body_limit",
        "redirect_header", "redirect_rejected", "parse_failure", "accepted", "obsolete"};
    auto* root = cJSON_CreateObject();
    cJSON_AddBoolToObject(root, "stationOnline", online);
    cJSON_AddNumberToObject(root, "epochSeconds", epochSeconds);
    cJSON_AddBoolToObject(root, "clockReady", epochSeconds >= 1700000000);
    cJSON_AddNumberToObject(root, "currentGeneration", current.generation);
    cJSON_AddBoolToObject(root, "weatherConfigured", current.configured);
    auto* attempt = cJSON_AddObjectToObject(root, "attempt");
    cJSON_AddBoolToObject(attempt, "attempted", a.attempted);
    cJSON_AddBoolToObject(attempt, "currentGeneration", a.attempted && a.generation == current.generation);
    number(attempt, "generation", a.attempted ? std::optional<uint64_t>(a.generation) : std::nullopt);
    number(attempt, "startedMs", a.attempted ? std::optional<uint64_t>(a.startedMs) : std::nullopt);
    number(attempt, "elapsedMs", a.attempted ? std::optional<uint64_t>(a.outcome == WeatherOutcome::Running ? now - a.startedMs : a.elapsedMs) : std::nullopt);
    number(attempt, "nextDueMs", a.attempted && a.outcome != WeatherOutcome::Running ? std::optional<uint64_t>(a.nextDueMs) : std::nullopt);
    if (a.attempted) cJSON_AddBoolToObject(attempt, "clockReady", a.clockReady); else cJSON_AddNullToObject(attempt, "clockReady");
    cJSON_AddStringToObject(attempt, "phase", phases[static_cast<unsigned>(a.phase)]);
    cJSON_AddStringToObject(attempt, "outcome", outcomes[static_cast<unsigned>(a.outcome)]);
    cJSON_AddStringToObject(attempt, "parserReason", parserNames[static_cast<unsigned>(a.parserReason)]);
    number(attempt, "httpResult", a.httpResult); number(attempt, "httpStatus", a.httpStatus);
    number(attempt, "espError", a.espError); number(attempt, "tlsCode", a.tlsCode);
    number(attempt, "verifyFlags", a.verifyFlags); number(attempt, "socketErrno", a.socketErrno);
    heap(attempt, "beforeHttp", a.beforeHttp); heap(attempt, "beforeTls", a.beforeTls); heap(attempt, "failureHeap", a.failureHeap);
    cJSON_AddNumberToObject(attempt, "wireBytes", a.wireBytes); cJSON_AddNumberToObject(attempt, "bodyBytes", a.bodyBytes);
    cJSON_AddNumberToObject(attempt, "redirects", a.redirects);
    auto* weather = cJSON_AddObjectToObject(root, "weather"); const auto& w = current.weather;
    cJSON_AddBoolToObject(weather, "hasData", w.data.has_value());
    cJSON_AddBoolToObject(weather, "unknown", w.unknown); cJSON_AddBoolToObject(weather, "stale", w.stale);
    number(weather, "ageMs", w.data ? std::optional<uint64_t>(now >= w.receivedAtMs ? now - w.receivedAtMs : 0) : std::nullopt);
    number(weather, "temperature", w.data ? w.data->temperature : std::nullopt);
    number(weather, "humidity", w.data ? w.data->humidity : std::nullopt);
    number(weather, "weatherCode", w.data ? w.data->weatherCode : std::nullopt);
    cJSON_AddNumberToObject(weather, "hourlyCount", w.data ? w.data->hourlyCount : 0);
    char* encoded = cJSON_PrintUnformatted(root);
    std::string result = encoded ? encoded : "{}";
    std::free(encoded); cJSON_Delete(root); return result;
}
}
