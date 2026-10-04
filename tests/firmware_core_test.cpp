#include "../firmware/main/core.hpp"

#include <cmath>
#include <fstream>
#include <iostream>
#include <sstream>
#include <stdexcept>
#include <string>

namespace {

int checks = 0;

void check(bool condition, const char* expression, int line) {
    ++checks;
    if (!condition) {
        std::ostringstream message;
        message << "check failed at line " << line << ": " << expression;
        throw std::runtime_error(message.str());
    }
}

#define CHECK(expression) check(static_cast<bool>(expression), #expression, __LINE__)

std::string readFile(const std::string& path) {
    std::ifstream input(path, std::ios::binary);
    if (!input) {
        throw std::runtime_error("cannot open fixture: " + path);
    }
    return std::string(std::istreambuf_iterator<char>(input), std::istreambuf_iterator<char>());
}

bool closeTo(double actual, double expected) {
    return std::fabs(actual - expected) < 0.000001;
}

std::string escapedHeaderPatch(const std::string& sourceName, const std::string& url) {
    std::string json = "{\"" + sourceName + "\":{\"url\":\"" + url + "\",\"headers\":{";
    for (int index = 0; index < 6; ++index) {
        if (index != 0) json += ',';
        json += "\"X-" + std::to_string(index) + "\":\"";
        for (int quote = 0; quote < 256; ++quote) json += "\\\"";
        json += '"';
    }
    json += "}}}";
    return json;
}

void usageContract(const std::string& root) {
    const auto fixture = readFile(root + "/examples/usage-response.json");
    const auto usage = deskduo::parseUsage(fixture);
    CHECK(usage.value.has_value());
    CHECK(usage.error.empty());
    CHECK(usage.value->provider == "codex");
    CHECK(usage.value->primary.has_value());
    CHECK(closeTo(usage.value->primary->usedPercent, 42.0));
    CHECK(usage.value->primary->windowMinutes == 300U);
    CHECK(usage.value->secondary.has_value());
    CHECK(closeTo(usage.value->secondary->usedPercent, 71.5));

    const auto absent = deskduo::parseUsage(
        R"({"schemaVersion":1,"type":"usage","provider":"claude","updatedAt":"2026-10-04T03:00:00Z"})");
    CHECK(absent.value.has_value());
    CHECK(!absent.value->primary.has_value());
    CHECK(!absent.value->secondary.has_value());

    const auto zero = deskduo::parseUsage(
        R"({"schemaVersion":1,"type":"usage","provider":"claude","updatedAt":"2024-02-29T12:00:00.250+07:00","primary":{"usedPercent":0}})");
    CHECK(zero.value.has_value());
    CHECK(closeTo(zero.value->primary->usedPercent, 0.0));
    CHECK(zero.value->updatedAt == 1709182800LL);

    const auto fraction = deskduo::parseUsage(
        R"({"schemaVersion":1,"type":"usage","provider":"codex","updatedAt":"2026-10-04T03:00:00Z","primary":{"usedPercent":1e2,"windowMinutes":0.5}})");
    CHECK(fraction.value.has_value());
    CHECK(fraction.value->primary->windowMinutes.has_value());
    CHECK(closeTo(*fraction.value->primary->windowMinutes, 0.5));
    const auto largeWindow = deskduo::parseUsage(
        R"({"schemaVersion":1,"type":"usage","provider":"codex","updatedAt":"2026-10-04T03:00:00Z","primary":{"usedPercent":100,"windowMinutes":1e100}})");
    CHECK(largeWindow.value.has_value());
    CHECK(largeWindow.value->primary->windowMinutes.has_value());
    CHECK(closeTo(*largeWindow.value->primary->windowMinutes / 1e100, 1.0));
    CHECK(!deskduo::parseUsage(
        R"({"schemaVersion":1,"type":"usage","provider":"codex","updatedAt":"2026-10-04T03:00:00Z","primary":{"usedPercent":100,"windowMinutes":0}})").value.has_value());
    CHECK(!deskduo::parseUsage(
        R"({"schemaVersion":1,"type":"usage","provider":"codex","updatedAt":"2026-10-04T03:00:00Z","primary":{"usedPercent":100,"windowMinutes":-0.5}})").value.has_value());
    CHECK(!deskduo::parseUsage(
        R"({"schemaVersion":1,"type":"usage","provider":"codex","updatedAt":"2026-10-04T03:00:00Z","primary":{"usedPercent":01}})").value.has_value());
    CHECK(!deskduo::parseUsage(
        R"({"schemaVersion":1,"type":"usage","provider":"codex","updatedAt":"2026-10-04T03:00:00Z","primary":{"usedPercent":1.}})").value.has_value());
    CHECK(!deskduo::parseUsage(
        R"({"schemaVersion":1,"type":"usage","provider":"codex","updatedAt":"2026-10-04T03:00:00Z","primary":{"usedPercent":1e+}})").value.has_value());

    CHECK(!deskduo::parseUsage("{").value.has_value());
    CHECK(deskduo::parseUsage(std::string(R"({"schemaVersion":1,"type":"usage","provider":"codex","updatedAt":"2026-10-04T03:00:00Z"})") + " \t\r\n").value.has_value());
    CHECK(!deskduo::parseUsage(R"({"schemaVersion":1,"type":"usage","provider":"codex","updatedAt":"2026-10-04T03:00:00Z"} garbage)").value.has_value());
    CHECK(!deskduo::parseUsage(R"({"schemaVersion":2,"type":"usage","provider":"codex","updatedAt":"2026-10-04T03:00:00Z"})").value.has_value());
    CHECK(!deskduo::parseUsage(R"({"schemaVersion":1,"type":"usage","provider":"other","updatedAt":"2026-10-04T03:00:00Z"})").value.has_value());
    CHECK(!deskduo::parseUsage(R"({"schemaVersion":true,"type":"usage","provider":"codex","updatedAt":"2026-10-04T03:00:00Z"})").value.has_value());
    CHECK(!deskduo::parseUsage(R"({"schemaVersion":1,"type":"usage","provider":"codex","updatedAt":"2026-10-04T03:00:00Z","primary":{"usedPercent":true}})").value.has_value());
    CHECK(!deskduo::parseUsage(R"({"schemaVersion":1,"type":"usage","provider":"codex","updatedAt":"2026-10-04T03:00:00Z","primary":{"usedPercent":100.1}})").value.has_value());
    CHECK(!deskduo::parseUsage(R"({"schemaVersion":1,"type":"usage","provider":"codex","updatedAt":"2026-02-29T03:00:00Z"})").value.has_value());
    CHECK(!deskduo::parseUsage(R"({"schemaVersion":1,"type":"usage","provider":"codex","updatedAt":"2026-10-04T03:00:00"})").value.has_value());
    CHECK(!deskduo::parseUsage(R"({"schemaVersion":1,"type":"usage","provider":"codex","updatedAt":"2026-10-04T03:00:00+15:00"})").value.has_value());
    CHECK(!deskduo::parseUsage(R"({"schemaVersion":1,"type":"usage","provider":"codex","updatedAt":"2026-10-04T03:00:00Z","primary":{"usedPercent":1e400}})").value.has_value());
    CHECK(!deskduo::parseUsage(R"({"schemaVersion":1,"type":"usage","provider":"codex\u0000x","updatedAt":"2026-10-04T03:00:00Z"})").value.has_value());

    std::string tooDeep = "{";
    for (int i = 0; i < 17; ++i) tooDeep += "\"x\":{";
    tooDeep += "\"leaf\":0";
    for (int i = 0; i < 18; ++i) tooDeep += "}";
    CHECK(!deskduo::parseUsage(tooDeep).value.has_value());
}

void stocksContract(const std::string& root) {
    const auto fixture = readFile(root + "/examples/stocks-response.json");
    const auto stocks = deskduo::parseStocks(fixture);
    CHECK(stocks.value.has_value());
    CHECK(stocks.value->count == 2U);
    CHECK(stocks.value->items[0].symbol == "PTT");
    CHECK(closeTo(*stocks.value->items[0].price, 32.5));
    CHECK(!stocks.value->items[1].changePercent.has_value());

    const auto zero = deskduo::parseStocks(
        R"({"schemaVersion":1,"generatedAt":"2026-10-04T03:00:00Z","items":[{"symbol":"PTT","price":0,"currency":"THB"}]})");
    CHECK(zero.value.has_value());
    CHECK(closeTo(*zero.value->items[0].price, 0.0));

    const auto largeFinite = deskduo::parseStocks(
        R"({"schemaVersion":1,"generatedAt":"2026-10-04T03:00:00Z","items":[{"symbol":"BIG","price":1e20,"changePercent":2000},{"symbol":"NEG","price":0,"changePercent":-2000}]})");
    CHECK(largeFinite.value.has_value());
    CHECK(closeTo(*largeFinite.value->items[0].price / 1e20, 1.0));
    CHECK(closeTo(*largeFinite.value->items[0].changePercent, 2000.0));
    CHECK(closeTo(*largeFinite.value->items[1].changePercent, -2000.0));
    CHECK(!deskduo::parseStocks(
        R"({"schemaVersion":1,"generatedAt":"2026-10-04T03:00:00Z","items":[{"symbol":"PTT","price":1.}]})").value.has_value());

    const auto missing = deskduo::parseStocks(
        R"({"schemaVersion":1,"generatedAt":"2026-10-04T03:00:00Z","items":[{"symbol":"PTT","currency":"THB"}]})");
    CHECK(missing.value.has_value());
    CHECK(!missing.value->items[0].price.has_value());

    std::string thaiLabel;
    for (int index = 0; index < 24; ++index) thaiLabel += "ก";
    const auto thai = deskduo::parseStocks(
        R"({"schemaVersion":1,"generatedAt":"2026-10-04T03:00:00Z","items":[{"symbol":"PTT","label":")" + thaiLabel + R"("}]})");
    CHECK(thai.value.has_value());
    CHECK(thai.value->items[0].label == thaiLabel);
    CHECK(thaiLabel.size() > 64U);

    CHECK(!deskduo::parseStocks(R"({"schemaVersion":1,"generatedAt":"2026-10-04T03:00:00Z","items":[{"symbol":"PTT","price":-0.01}]})").value.has_value());
    CHECK(!deskduo::parseStocks(R"({"schemaVersion":1,"generatedAt":"2026-10-04T03:00:00Z","items":[{"symbol":"PTT","price":true}]})").value.has_value());
    CHECK(!deskduo::parseStocks(R"({"schemaVersion":1,"generatedAt":"2026-10-04T03:00:00Z","items":[{},{},{},{},{},{}]})").value.has_value());
    CHECK(!deskduo::parseStocks(R"({"schemaVersion":1,"error":"upstream","items":[]})").value.has_value());
    CHECK(!deskduo::parseStocks(R"({"schemaVersion":1,"generatedAt":"2026-10-04T03:00:00Z","items":[{"symbol":"PTT\n"}]})").value.has_value());
}

void weatherContract(const std::string& root) {
    auto source = deskduo::defaultConfig().weather;
    const auto simulated = deskduo::parseWeather(readFile(root + "/examples/weather-response.json"), source);
    CHECK(simulated.value.has_value());
    CHECK(simulated.value->hourlyCount == 4U);
    CHECK(!deskduo::parseWeather(R"({})", source).value.has_value());
    CHECK(!deskduo::parseWeather(R"({"error":true,"reason":"invalid location"})", source).value.has_value());
    const auto explicitNoError = deskduo::parseWeather(
        R"({"error":false,"current":{"temperature_2m":1}})", source);
    CHECK(explicitNoError.value.has_value());
    CHECK(explicitNoError.value->temperature.has_value());

    const auto weather = deskduo::parseWeather(
        R"({"current":{"temperature_2m":0,"relative_humidity_2m":0,"weather_code":0,"time":"2026-10-04T12:00","rain_probability":0},"utc_offset_seconds":25200,"hourly":{"time":["2026-10-04T12:00","2026-10-04T13:00","2026-10-04T14:00","2026-10-04T15:00"],"temperature_2m":[0,-1,2,3],"precipitation_probability":[0,10,20,30]}})", source);
    CHECK(weather.value.has_value());
    CHECK(weather.value->temperature.has_value() && closeTo(*weather.value->temperature, 0.0));
    CHECK(weather.value->humidity.has_value() && closeTo(*weather.value->humidity, 0.0));
    CHECK(weather.value->weatherCode == 0);
    CHECK(weather.value->updatedAt.has_value());
    CHECK(*weather.value->updatedAt == 1791090000LL);
    CHECK(weather.value->hourlyCount == 4U);
    CHECK(weather.value->hourlyTemperatures[1].has_value() && closeTo(*weather.value->hourlyTemperatures[1], -1.0));
    CHECK(weather.value->hourlyRainProbability[3].has_value() && closeTo(*weather.value->hourlyRainProbability[3], 30.0));

    const auto unknownLocalTime = deskduo::parseWeather(
        R"({"current":{"temperature_2m":-2.5,"relative_humidity_2m":100,"weather_code":99,"time":"2026-10-04T12:00"}})", source);
    CHECK(unknownLocalTime.value.has_value());
    CHECK(closeTo(*unknownLocalTime.value->temperature, -2.5));
    CHECK(!unknownLocalTime.value->updatedAt.has_value());
    CHECK(!unknownLocalTime.value->humidity || closeTo(*unknownLocalTime.value->humidity, 100.0));
    CHECK(unknownLocalTime.value->hourlyCount == 0U);

    CHECK(!deskduo::parseWeather(R"({"current":{"weather_code":4}})", source).value.has_value());
    CHECK(!deskduo::parseWeather(R"({"current":{"relative_humidity_2m":100.1}})", source).value.has_value());
    CHECK(!deskduo::parseWeather(R"({"current":{"temperature_2m":true}})", source).value.has_value());
    CHECK(!deskduo::parseWeather(R"({"current":{"time":"2026-10-04T12:00.5"}})", source).value.has_value());

    auto indexed = source;
    indexed.fields["temperature"] = "measurements[0].temperature";
    const auto indexValue = deskduo::parseWeather(R"({"measurements":[{"temperature":14.25}]})", indexed);
    CHECK(indexValue.value.has_value());
    CHECK(indexValue.value->temperature.has_value() && closeTo(*indexValue.value->temperature, 14.25));
    indexed.fields["temperature"] = "measurements[abc].temperature";
    CHECK(!deskduo::parseWeather(R"({"measurements":[{"temperature":14.25}]})", indexed).value.has_value());
}

void configContract(const std::string& root) {
    auto config = deskduo::defaultConfig();
    CHECK(config.weather.url.empty());
    CHECK(config.weather.refreshSeconds == 900U);
    CHECK(config.stocks.refreshSeconds == 300U);
    CHECK(config.autoRotateSeconds == 20U);
    CHECK(config.weather.label == "Weather");
    CHECK(config.weather.fields.at("temperature") == "current.temperature_2m");
    CHECK(deskduo::applyConfigPatch(config, R"({"weather":{"refreshSeconds":30}})").value.has_value());
    CHECK(deskduo::applyConfigPatch(config, R"({"stocks":{"refreshSeconds":86400}})").value.has_value());
    CHECK(!deskduo::applyConfigPatch(config, R"({"weather":{"refreshSeconds":29}})").value.has_value());
    CHECK(!deskduo::applyConfigPatch(config, R"({"stocks":{"refreshSeconds":86401}})").value.has_value());
    CHECK(!deskduo::applyConfigPatch(config, R"({"weather":{"refreshSeconds":30.5}})").value.has_value());
    CHECK(deskduo::applyConfigPatch(config, R"({"autoRotateSeconds":0})").value.has_value());
    CHECK(deskduo::applyConfigPatch(config, R"({"autoRotateSeconds":5})").value.has_value());
    CHECK(deskduo::applyConfigPatch(config, R"({"autoRotateSeconds":300})").value.has_value());
    CHECK(!deskduo::applyConfigPatch(config, R"({"autoRotateSeconds":1})").value.has_value());
    CHECK(!deskduo::applyConfigPatch(config, R"({"autoRotateSeconds":2})").value.has_value());
    CHECK(!deskduo::applyConfigPatch(config, R"({"autoRotateSeconds":301})").value.has_value());
    CHECK(!deskduo::applyConfigPatch(config, R"({"autoRotateSeconds":5.5})").value.has_value());
    const auto example = deskduo::applyConfigPatch(config, readFile(root + "/examples/device-config.example.json"));
    CHECK(example.value.has_value());
    CHECK(example.value->weather.url.empty());
    CHECK(example.value->stocks.url.empty());

    const auto first = deskduo::applyConfigPatch(config,
        R"({"ssid":"home","wifiPassword":"wifi-pass","usageToken":"0123456789abcdef","weather":{"url":"https://weather.example/v1?key=weather-secret","headers":{"X-Feed":"feed-secret"},"refreshSeconds":600,"label":"Bangkok"},"stocks":{"url":"https://stocks.example/v1?accessToken=stocks-secret"},"autoRotateSeconds":30,"touch":{"swapAxes":true,"invertX":false,"invertY":true,"minX":100,"maxX":3900,"minY":200,"maxY":3800}})");
    CHECK(first.value.has_value());
    CHECK(first.value->ssid == "home");
    CHECK(first.value->weather.label == "Bangkok");
    CHECK(first.value->weather.refreshSeconds == 600U);
    CHECK(first.value->weather.fields.at("temperature") == "current.temperature_2m");
    CHECK(first.value->touch.swapAxes);

    CHECK(!deskduo::applyConfigPatch(config, std::string(8193, ' ')).value.has_value());
    std::string maximumUrl = "https://w.example/";
    maximumUrl += std::string(1020 - maximumUrl.size(), 'a');
    CHECK(maximumUrl.size() == 1020U);
    const auto fullWeatherPatch = deskduo::applyConfigPatch(config, escapedHeaderPatch("weather", maximumUrl));
    CHECK(fullWeatherPatch.value.has_value());
    const auto fullStocksPatch = deskduo::applyConfigPatch(*fullWeatherPatch.value, escapedHeaderPatch("stocks", maximumUrl));
    CHECK(fullStocksPatch.value.has_value());
    const std::string maximalStored = deskduo::serializeConfig(*fullStocksPatch.value);
    CHECK(maximalStored.size() > 8192U);
    CHECK(maximalStored.size() <= 32768U);
    const auto maximalRestore = deskduo::parseStoredConfig(maximalStored);
    CHECK(maximalRestore.value.has_value());
    CHECK(maximalRestore.value->weather.headers.size() == 6U);
    CHECK(maximalRestore.value->weather.headers.at("X-0") == std::string(256, '"'));
    CHECK(maximalRestore.value->stocks.headers.at("X-5") == std::string(256, '"'));
    const auto partial = deskduo::applyConfigPatch(*first.value, R"({"weather":{"label":"Forecast"}})");
    CHECK(partial.value.has_value());
    CHECK(partial.value->ssid == "home");
    CHECK(partial.value->weather.url == first.value->weather.url);
    CHECK(partial.value->weather.label == "Forecast");

    CHECK(!deskduo::applyConfigPatch(config, "{").value.has_value());
    CHECK(!deskduo::applyConfigPatch(config, "{\"weather\":{\"url\":\"" + std::string(1025, 'a') + "\"}}").value.has_value());
    CHECK(!deskduo::applyConfigPatch(config, R"({"weather":{"url":"https://example.test/\r\nInjected: yes"}})").value.has_value());
    CHECK(!deskduo::applyConfigPatch(config, R"({"weather":{"url":"https://user:pass@example.test/feed"}})").value.has_value());
    CHECK(!deskduo::applyConfigPatch(config, R"({"weather":{"headers":{"X-Test":"bad\r\nvalue"}}})").value.has_value());
    CHECK(!deskduo::applyConfigPatch(config, R"({"weather":{"headers":{"X-One":"1","X-Two":"2","X-Three":"3","X-Four":"4","X-Five":"5","X-Six":"6","X-Seven":"7"}}})").value.has_value());
    CHECK(!deskduo::applyConfigPatch(config, "{\"weather\":{\"headers\":{\"X-Large\":\"" + std::string(257, 'x') + "\"}}}").value.has_value());
    CHECK(!deskduo::applyConfigPatch(config, R"({"usageToken":"short"})").value.has_value());
    CHECK(!deskduo::applyConfigPatch(config, R"({"wifiPassword":"bad\u0000secret"})").value.has_value());
    CHECK(!deskduo::applyConfigPatch(config, R"({"autoRotateSeconds":1})").value.has_value());
    CHECK(!deskduo::applyConfigPatch(config, R"({"stocks":{"refreshSeconds":1}})").value.has_value());
    CHECK(!deskduo::applyConfigPatch(config, R"({"setupPassword":"unauthorized"})").value.has_value());
    CHECK(!deskduo::applyConfigPatch(config, R"({"touch":{"minX":100,"maxX":100}})").value.has_value());
    CHECK(!deskduo::applyConfigPatch(config, R"({"weather":{"url":"ftp://weather.example/feed"}})").value.has_value());
    CHECK(!deskduo::applyConfigPatch(config, R"({"weather":{"fields":{"temperature":"current..bad"}}})").value.has_value());

    const std::string serialized = deskduo::serializeConfig(*first.value);
    const auto restored = deskduo::parseStoredConfig(serialized);
    CHECK(restored.value.has_value());
    CHECK(restored.value->wifiPassword == "wifi-pass");
    CHECK(restored.value->usageToken == "0123456789abcdef");
    CHECK(restored.value->weather.url == first.value->weather.url);

    auto internal = *first.value;
    internal.setupPassword = "generated-setup-password";
    const auto withGeneratedPassword = deskduo::parseStoredConfig(deskduo::serializeConfig(internal));
    CHECK(withGeneratedPassword.value.has_value());
    CHECK(withGeneratedPassword.value->setupPassword == "generated-setup-password");
    const std::string redacted = deskduo::redactedConfig(internal);
    CHECK(redacted.find("wifi-pass") == std::string::npos);
    CHECK(redacted.find("0123456789abcdef") == std::string::npos);
    CHECK(redacted.find("generated-setup-password") == std::string::npos);
    CHECK(redacted.find("feed-secret") == std::string::npos);
    CHECK(redacted.find("weather-secret") == std::string::npos);
    CHECK(redacted.find("stocks-secret") == std::string::npos);
    CHECK(redacted.find("key=weather-secret") == std::string::npos);
    CHECK(redacted.find("https://weather.example/v1") != std::string::npos);
}

void modelContract() {
    auto config = deskduo::defaultConfig();
    deskduo::Model model;
    model.applyConfig(config);
    auto snapshot = model.snapshot(0);
    CHECK(snapshot.stocks.unknown);
    CHECK(snapshot.weather.unknown);

    model.markFailed("stocks", model.generation());
    snapshot = model.snapshot(1000);
    CHECK(snapshot.stocks.unknown);
    CHECK(!snapshot.stocks.data.has_value());

    auto stocks = deskduo::parseStocks(
        R"({"schemaVersion":1,"generatedAt":"2026-10-04T03:00:00Z","items":[{"symbol":"PTT","price":32.5}]})");
    CHECK(stocks.value.has_value());
    const auto gen = model.generation();
    model.acceptStocks(*stocks.value, 1000, gen);
    model.markFailed("stocks", gen);
    snapshot = model.snapshot(1001);
    CHECK(!snapshot.stocks.unknown);
    CHECK(snapshot.stocks.stale);
    CHECK(snapshot.stocks.data->items[0].symbol == "PTT");

    config.stocks.url = "https://stocks.example/new";
    model.applyConfig(config);
    CHECK(model.generation() > gen);
    snapshot = model.snapshot(1002);
    CHECK(snapshot.stocks.unknown);
    CHECK(!snapshot.stocks.data.has_value());
    model.acceptStocks(*stocks.value, 1003, gen);
    CHECK(model.snapshot(1003).stocks.unknown);

    auto currentConfig = config;
    model.applyConfig(currentConfig);
    const auto currentGen = model.generation();
    model.acceptStocks(*stocks.value, 2000, currentGen);
    snapshot = model.snapshot(2000 + 2U * currentConfig.stocks.refreshSeconds * 1000U);
    CHECK(snapshot.stocks.stale);
    CHECK(!snapshot.stocks.unknown);

    auto weather = deskduo::parseWeather(R"({"current":{"temperature_2m":22,"weather_code":1}})", currentConfig.weather);
    CHECK(weather.value.has_value());
    model.acceptWeather(*weather.value, 3000, currentGen);
    snapshot = model.snapshot(3001);
    CHECK(!snapshot.weather.unknown);
    CHECK(!snapshot.weather.stale);
    const auto errorEnvelope = deskduo::parseWeather(
        R"({"error":true,"reason":"invalid location"})", currentConfig.weather);
    CHECK(!errorEnvelope.value.has_value());
    model.markFailed("weather", currentGen);
    snapshot = model.snapshot(3002);
    CHECK(!snapshot.weather.unknown);
    CHECK(snapshot.weather.stale);
    CHECK(snapshot.weather.data->temperature.has_value());
    CHECK(closeTo(*snapshot.weather.data->temperature, 22.0));

    currentConfig.weather.url = "https://weather.example/new";
    model.applyConfig(currentConfig);
    const auto weatherGeneration = model.generation();
    snapshot = model.snapshot(3002);
    CHECK(snapshot.weather.unknown);
    CHECK(!snapshot.weather.data.has_value());
    CHECK(!snapshot.stocks.unknown);
    model.acceptWeather(*weather.value, 3003, currentGen);
    CHECK(model.snapshot(3003).weather.unknown);
    model.acceptWeather(*weather.value, 3004, weatherGeneration);
    CHECK(!model.snapshot(3004).weather.unknown);

    deskduo::Usage claude;
    claude.provider = "claude";
    claude.updatedAt = 100;
    claude.primary = deskduo::UsageWindow{42.0, std::nullopt, std::nullopt};
    deskduo::Usage codex;
    codex.provider = "codex";
    codex.updatedAt = 90;
    codex.primary = deskduo::UsageWindow{20.0, std::nullopt, std::nullopt};
    model.acceptUsage(claude, 4000);
    model.acceptUsage(codex, 4001);
    auto olderClaude = claude;
    olderClaude.updatedAt = 99;
    olderClaude.primary = deskduo::UsageWindow{1.0, std::nullopt, std::nullopt};
    model.acceptUsage(olderClaude, 4002);
    snapshot = model.snapshot(4002);
    CHECK(snapshot.claude.data.has_value());
    CHECK(closeTo(snapshot.claude.data->primary->usedPercent, 42.0));
    CHECK(snapshot.codex.data.has_value());
    CHECK(closeTo(snapshot.codex.data->primary->usedPercent, 20.0));
    CHECK(snapshot.claude.receivedAtMs == 4000U);
}

void redirectAndTouchContract() {
    CHECK(deskduo::redirectHeadersAllowed("https://feed.example/a", "https://feed.example/b"));
    CHECK(deskduo::redirectHeadersAllowed("https://feed.example/a", "https://FEED.example:443/b"));
    CHECK(!deskduo::redirectHeadersAllowed("https://feed.example/a", "https://other.example/b"));
    CHECK(!deskduo::redirectHeadersAllowed("https://feed.example/a", "http://feed.example/b"));
    CHECK(deskduo::redirectTargetAllowed("http://feed.example/a", "https://feed.example/b"));
    CHECK(!deskduo::redirectTargetAllowed("https://feed.example/a", "http://feed.example/b"));
    CHECK(!deskduo::redirectTargetAllowed("https://feed.example/a", "file:///tmp/x"));
    CHECK(!deskduo::redirectTargetAllowed("https://feed.example/a", "https://user:secret@feed.example/a"));
    CHECK(!deskduo::redirectTargetAllowed("https://feed.example/a", "https://feed.example/a\r\nX: y"));

    deskduo::TouchCalibration calibration;
    CHECK(deskduo::mapTouch(0, 0, calibration) == std::make_pair(0, 0));
    CHECK(deskduo::mapTouch(4095, 0, calibration) == std::make_pair(239, 0));
    CHECK(deskduo::mapTouch(0, 4095, calibration) == std::make_pair(0, 319));
    CHECK(deskduo::mapTouch(4095, 4095, calibration) == std::make_pair(239, 319));
    CHECK(deskduo::mapTouch(-50, 5000, calibration) == std::make_pair(0, 319));
    calibration.invertX = true;
    calibration.invertY = true;
    CHECK(deskduo::mapTouch(0, 0, calibration) == std::make_pair(239, 319));
    calibration.invertX = false;
    calibration.invertY = false;
    calibration.swapAxes = true;
    CHECK(deskduo::mapTouch(0, 4095, calibration) == std::make_pair(239, 0));
}

}  // namespace

int main(int argc, char** argv) {
    if (argc != 2) {
        std::cerr << "usage: firmware_core_test REPOSITORY_ROOT\n";
        return 2;
    }
    try {
        const std::string root = argv[1];
        usageContract(root);
        stocksContract(root);
        weatherContract(root);
        configContract(root);
        modelContract();
        redirectAndTouchContract();
        std::cout << "firmware core checks passed: " << checks << "\n";
        return 0;
    } catch (const std::exception& error) {
        std::cerr << error.what() << "\n";
        return 1;
    }
}
