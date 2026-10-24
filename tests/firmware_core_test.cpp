#include "../firmware/main/core.hpp"
#include "../firmware/main/portal.hpp"
#include "../firmware/main/network.hpp"
#include "../firmware/main/assets.hpp"
#include "../firmware/main/runtime.hpp"

#include "cJSON.h"
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
    CHECK(!deskduo::parseWeather(R"({"current":{"temperature_2m":25,"is_day":2}})", source).value);
    const auto simulated = deskduo::parseWeather(readFile(root + "/examples/weather-response.json"), source);
    CHECK(simulated.value.has_value());
    CHECK(simulated.value->hourlyCount == 3U);
    // The source observation at 12:45 excludes the past 12:00 sample.
    CHECK(simulated.value->updatedAt == 1791092700LL);
    CHECK(simulated.value->hourlyTimes[0] == 1791093600LL);
    CHECK(simulated.value->hourlyTimes[2] == 1791100800LL);
    CHECK(!simulated.value->hourlyTimes[3]);
    CHECK(simulated.value->hourlyTemperatures[0] == 31.7);
    CHECK(simulated.value->hourlyTemperatures[2] == 29.5);
    CHECK(simulated.value->hourlyRainProbability[0] == 10);
    CHECK(simulated.value->hourlyRainProbability[2] == 15);
    CHECK(simulated.value->isDay == true);
    CHECK(simulated.value->hourlyWeatherCodes[0] == 2);
    CHECK(simulated.value->hourlyWeatherCodes[2] == 61);
    CHECK(simulated.value->hourlyIsDay[2] == true);
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

    // Legacy stored mappings omit new keys; defaults still fill them in.
    auto legacy = source;
    legacy.fields = {{"temperature", "current.temperature_2m"}, {"humidity", "current.relative_humidity_2m"},
        {"weatherCode", "current.weather_code"}, {"updatedAt", "current.time"},
        {"utcOffsetSeconds", "utc_offset_seconds"}, {"hourlyTimes", "hourly.time"},
        {"hourlyTemperatures", "hourly.temperature_2m"}, {"hourlyRainProbability", "hourly.precipitation_probability"}};
    const auto sliced = deskduo::parseWeather(R"({"current":{"temperature_2m":25,"is_day":0,"time":"2026-10-04T02:00Z"},"hourly":{"time":["2026-10-04T00:00Z","2026-10-04T01:00Z","2026-10-04T02:00Z","2026-10-04T03:00Z","2026-10-04T04:00Z","2026-10-04T05:00Z","2026-10-04T06:00Z"],"temperature_2m":[1,2,3,4,5,6,7],"precipitation_probability":[10,20,30,40,50,60,70],"weather_code":[0,1,2,3,61,95,99],"is_day":[0,0,0,1,true,false,1]}})", legacy);
    CHECK(sliced.value);
    CHECK(sliced.value->isDay == false);
    CHECK(!sliced.value->utcOffsetSeconds);
    CHECK(sliced.value->hourlyCount == 4);
    CHECK(sliced.value->hourlyTimes[0] == 1791079200LL);
    CHECK(sliced.value->hourlyTimes[3] == 1791090000LL);
    CHECK(sliced.value->hourlyTemperatures[0] == 3);
    CHECK(sliced.value->hourlyTemperatures[3] == 6);
    CHECK(sliced.value->hourlyRainProbability[0] == 30);
    CHECK(sliced.value->hourlyRainProbability[3] == 60);
    CHECK(sliced.value->hourlyWeatherCodes[0] == 2);
    CHECK(sliced.value->hourlyWeatherCodes[3] == 95);
    CHECK(sliced.value->hourlyIsDay[0] == false);
    CHECK(sliced.value->hourlyIsDay[1] == true);
    CHECK(sliced.value->hourlyIsDay[2] == true);
    CHECK(sliced.value->hourlyIsDay[3] == false);
    CHECK(!weather.value->isDay && !weather.value->hourlyWeatherCodes[0] && !weather.value->hourlyIsDay[0]);
    // Unknown offset must not turn local source times into UTC or discard them.
    const auto unknownForecast = deskduo::parseWeather(R"({"current":{"temperature_2m":25,"is_day":true,"time":"2026-10-04T02:00"},"hourly":{"time":["2026-10-04T00:00","2026-10-04T01:00"],"temperature_2m":[10,11],"weather_code":[null,3],"is_day":[null,0]}})", source);
    CHECK(unknownForecast.value && !unknownForecast.value->updatedAt);
    CHECK(unknownForecast.value->isDay == true);
    CHECK(unknownForecast.value->hourlyCount == 2);
    CHECK(!unknownForecast.value->hourlyTimes[0]);
    CHECK(unknownForecast.value->hourlyTemperatures[0] == 10);
    CHECK(!unknownForecast.value->hourlyWeatherCodes[0]);
    CHECK(unknownForecast.value->hourlyWeatherCodes[1] == 3);
    CHECK(!unknownForecast.value->hourlyIsDay[0]);
    CHECK(unknownForecast.value->hourlyIsDay[1] == false);
    const auto unequal = deskduo::parseWeather(R"({"current":{"temperature_2m":25},"hourly":{"temperature_2m":[null,12,13],"weather_code":[61],"is_day":[false,null]}})", source);
    CHECK(unequal.value && unequal.value->hourlyCount == 3);
    CHECK(unequal.value->hourlyWeatherCodes[0] == 61 && !unequal.value->hourlyWeatherCodes[1]);
    CHECK(unequal.value->hourlyIsDay[0] == false && !unequal.value->hourlyIsDay[1]);
    CHECK(!unequal.value->hourlyTemperatures[0] && unequal.value->hourlyTemperatures[2] == 13);
    for (const auto* invalid : {"2", "-1", "0.5", "\"1\"", "{}", "[]"}) {
        CHECK(!deskduo::parseWeather(std::string("{\"current\":{\"temperature_2m\":25,\"is_day\":") + invalid + "}}", source).value);
        CHECK(!deskduo::parseWeather(std::string("{\"current\":{\"temperature_2m\":25},\"hourly\":{\"is_day\":[") + invalid + "]}}", source).value);
    }
    for (const auto* invalid : {"4", "-1", "100", "1.5", "true", "\"61\"", "{}"}) {
        CHECK(!deskduo::parseWeather(std::string("{\"current\":{\"temperature_2m\":25},\"hourly\":{\"weather_code\":[") + invalid + "]}}", source).value);
    }
    CHECK(!deskduo::parseWeather(R"({"current":{"temperature_2m":25},"hourly":{"time":["bad"],"weather_code":[0]}})", source).value);
    CHECK(!deskduo::parseWeather(R"({"current":{"temperature_2m":25},"hourly":{"time":["2026-10-04T03:00Z","2026-10-04T02:00Z"]}})", source).value);
    auto custom = source;
    custom.fields["isDay"] = "day";
    custom.fields["hourlyWeatherCodes"] = "codes";
    custom.fields["hourlyIsDay"] = "days";
    const auto remapped = deskduo::parseWeather(R"({"day":false,"codes":[61,99],"days":[1,0]})", custom);
    CHECK(remapped.value && remapped.value->isDay == false);
    CHECK(remapped.value->hourlyWeatherCodes[1] == 99 && remapped.value->hourlyIsDay[1] == false);
    const auto configWithMappings = deskduo::applyConfigPatch(deskduo::defaultConfig(), R"({"weather":{"fields":{"temperature":"current.temperature_2m","humidity":"current.relative_humidity_2m","weatherCode":"current.weather_code","updatedAt":"current.time","utcOffsetSeconds":"utc_offset_seconds","hourlyTimes":"hourly.time","hourlyTemperatures":"hourly.temperature_2m","hourlyRainProbability":"hourly.precipitation_probability","isDay":"current.is_day","hourlyWeatherCodes":"hourly.weather_code","hourlyIsDay":"hourly.is_day"}}})");
    CHECK(configWithMappings.value && configWithMappings.value->weather.fields.size() == 11);
    CHECK(deskduo::parseStoredConfig(deskduo::serializeConfig(*configWithMappings.value)).value);

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

    // A public patch cannot clear a provisioned secret; omission preserves it.
    CHECK(!deskduo::applyConfigPatch(*first.value, R"({"usageToken":""})").value.has_value());
    CHECK(first.value->usageToken == "0123456789abcdef");
    CHECK(!deskduo::applyConfigPatch(config, R"({"usageToken":""})").value.has_value());
    const auto unprovisioned = deskduo::parseStoredConfig(deskduo::serializeConfig(config));
    CHECK(unprovisioned.value.has_value());
    CHECK(unprovisioned.value->usageToken.empty());

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
    CHECK(partial.value->usageToken == "0123456789abcdef");
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
    CHECK(redacted.find(R"("currentSSID":"home")") != std::string::npos);
    CHECK(redacted.find(R"("wifiPasswordConfigured":true)") != std::string::npos);
    CHECK(redacted.find(R"("usageTokenConfigured":true)") != std::string::npos);
    CHECK(redacted.find(R"("headersConfigured":true)") != std::string::npos);
    CHECK(redacted.find(R"("headersConfigured":false)") != std::string::npos);
    CHECK(redacted.find(R"("urlDetailsHidden":true)") != std::string::npos);
    CHECK(redacted.find(R"("wifiPassword":)") == std::string::npos);
    CHECK(redacted.find(R"("usageToken":)") == std::string::npos);
    CHECK(redacted.find(R"("setupPassword":)") == std::string::npos);
    CHECK(redacted.find(R"("headers":)") == std::string::npos);
    cJSON* publicJson = cJSON_Parse(redacted.c_str());
    CHECK(publicJson != nullptr);
    CHECK(cJSON_GetArraySize(publicJson) == 7);
    const cJSON* weatherMetadata = cJSON_GetObjectItemCaseSensitive(publicJson, "weather");
    const cJSON* stocksMetadata = cJSON_GetObjectItemCaseSensitive(publicJson, "stocks");
    CHECK(cJSON_IsTrue(cJSON_GetObjectItemCaseSensitive(weatherMetadata, "headersConfigured")));
    CHECK(cJSON_IsFalse(cJSON_GetObjectItemCaseSensitive(stocksMetadata, "headersConfigured")));
    CHECK(cJSON_IsTrue(cJSON_GetObjectItemCaseSensitive(weatherMetadata, "urlDetailsHidden")));
    CHECK(cJSON_IsTrue(cJSON_GetObjectItemCaseSensitive(stocksMetadata, "urlDetailsHidden")));
    CHECK(cJSON_GetArraySize(weatherMetadata) == 6);
    CHECK(cJSON_GetArraySize(stocksMetadata) == 7);
    cJSON_Delete(publicJson);


    // Only validated Open-Meteo coordinate pairs are public; all other query
    // values and headers stay private, and metadata is read-only.
    auto meteo = internal;
    meteo.weather.url = "https://api.open-meteo.com/v1/forecast?latitude=13.7563&longitude=100.5018&apikey=coordinate-secret";
    const auto meteoPublic = deskduo::redactedConfig(meteo);
    cJSON* meteoJson = cJSON_Parse(meteoPublic.c_str());
    const cJSON* meteoWeather = cJSON_GetObjectItemCaseSensitive(meteoJson, "weather");
    const cJSON* latitude = cJSON_GetObjectItemCaseSensitive(meteoWeather, "latitude");
    const cJSON* longitude = cJSON_GetObjectItemCaseSensitive(meteoWeather, "longitude");
    CHECK(cJSON_IsNumber(latitude) && latitude->valuedouble == 13.7563);
    CHECK(cJSON_IsNumber(longitude) && longitude->valuedouble == 100.5018);
    CHECK(meteoPublic.find("coordinate-secret") == std::string::npos);
    CHECK(meteoPublic.find("feed-secret") == std::string::npos);
    CHECK(meteoPublic.find("?latitude") == std::string::npos);
    cJSON_Delete(meteoJson);
    CHECK(deskduo::serializeConfig(meteo).find(R"("latitude":)") == std::string::npos);
    CHECK(!deskduo::applyConfigPatch(meteo, R"({"weather":{"latitude":13,"longitude":100}})").value.has_value());
    CHECK(!deskduo::parseStoredConfig(meteoPublic).value.has_value());
    const auto labelOnly = deskduo::applyConfigPatch(meteo, R"({"weather":{"label":"New label","refreshSeconds":1200}})");
    CHECK(labelOnly.value.has_value());
    CHECK(labelOnly.value->weather.url == meteo.weather.url);
    CHECK(labelOnly.value->weather.headers == meteo.weather.headers);
    for (const auto& url : std::vector<std::string>{
        "https://api.open-meteo.com/v1/forecast?latitude=91&longitude=100",
        "https://api.open-meteo.com/v1/forecast?latitude=13&longitude=-181",
        "https://api.open-meteo.com/v1/forecast?latitude=&longitude=100",
        "https://api.open-meteo.com/v1/forecast?latitude=13",
        "https://api.open-meteo.com/v1/forecast?latitude=NaN&longitude=100",
        "https://api.open-meteo.com/v1/forecast?latitude=inf&longitude=100",
        "https://api.open-meteo.com/v1/forecast?latitude=13junk&longitude=100",
        "https://api.open-meteo.com/v1/forecast?latitude=0x1&longitude=100",
        "https://api.open-meteo.com/v1/forecast?latitude=13&latitude=14&longitude=100",
        "https://api.open-meteo.com/v1/forecast?latitude=13&longitude=100&longitude=101",
        "https://api.open-meteo.com/v1/forecast?latitude=13&longitude=100&%6catitude=14",
        "https://api.open-meteo.com/v1/forecast?latitude=13&longitude=100#secret",
        "http://api.open-meteo.com/v1/forecast?latitude=13&longitude=100",
        "https://api.open-meteo.com:444/v1/forecast?latitude=13&longitude=100",
        "https://user@api.open-meteo.com/v1/forecast?latitude=13&longitude=100",
        "https://api.open-meteo.com.evil/v1/forecast?latitude=13&longitude=100",
        "https://api.open-meteo.com/v1/other?latitude=13&longitude=100"}) {
        meteo.weather.url = url;
        const auto output = deskduo::redactedConfig(meteo);
        CHECK(output.find(R"("latitude":)") == std::string::npos);
        CHECK(output.find(R"("longitude":)") == std::string::npos);
    }
    for (const auto& query : {"latitude=0&longitude=0", "latitude=-90&longitude=180", "latitude=90&longitude=-180"}) {
        meteo.weather.url = std::string("https://api.open-meteo.com/v1/forecast?") + query;
        CHECK(deskduo::redactedConfig(meteo).find(R"("latitude":)") != std::string::npos);
    }

    const auto unset = deskduo::redactedConfig(config);
    CHECK(unset.find(R"("currentSSID":"")") != std::string::npos);
    CHECK(unset.find(R"("wifiPasswordConfigured":false)") != std::string::npos);
    CHECK(unset.find(R"("usageTokenConfigured":false)") != std::string::npos);
    CHECK(unset.find(R"("headersConfigured":true)") == std::string::npos);
    CHECK(unset.find(R"("urlDetailsHidden":true)") == std::string::npos);
    CHECK(unset.find(R"("urlDetailsHidden":false)") != std::string::npos);
    auto fragment = config;
    fragment.stocks.url = "https://stocks.example/v1#private-fragment";
    CHECK(deskduo::redactedConfig(fragment).find("private-fragment") == std::string::npos);
    CHECK(deskduo::redactedConfig(fragment).find(R"("urlDetailsHidden":true)") != std::string::npos);

    // Literal bytes from the established persisted shape: public metadata must
    // never change records read by older firmware's strict stored parser.
    internal.weather.fields = {{"temperature", "current.temp"}};
    const std::string establishedBytes = R"({"ssid":"home","wifiPassword":"wifi-pass","usageToken":"0123456789abcdef","setupPassword":"generated-setup-password","weather":{"url":"https://weather.example/v1?key=weather-secret","refreshSeconds":600,"label":"Bangkok","fields":{"temperature":"current.temp"},"headers":{"X-Feed":"feed-secret"}},"stocks":{"url":"https://stocks.example/v1?accessToken=stocks-secret","refreshSeconds":300,"headers":{}},"touch":{"swapAxes":true,"invertX":false,"invertY":true,"minX":100,"maxX":3900,"minY":200,"maxY":3800},"autoRotateSeconds":30})";
    CHECK(deskduo::serializeConfig(internal) == establishedBytes);
    CHECK(deskduo::parseStoredConfig(establishedBytes).value.has_value());
    CHECK(!deskduo::applyConfigPatch(internal, R"({"currentSSID":"other"})").value.has_value());
    CHECK(!deskduo::parseStoredConfig(R"({"wifiPasswordConfigured":true})").value.has_value());
    const auto changedOnly = deskduo::applyConfigPatch(internal, R"({"touch":{"invertX":true},"weather":{"fields":{"temperature":"data.temp"}}})");
    CHECK(changedOnly.value.has_value());
    CHECK(changedOnly.value->weather.url == internal.weather.url);
    CHECK(changedOnly.value->weather.headers == internal.weather.headers);
    CHECK(changedOnly.value->wifiPassword == internal.wifiPassword);
    CHECK(changedOnly.value->usageToken == internal.usageToken);
    CHECK(changedOnly.value->touch.minX == internal.touch.minX);
    CHECK(changedOnly.value->touch.invertX);

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
    // Let browser tests consume the production public serializer, rather than
    // an accidentally unredacted GET fixture.
    if (argc == 3 && std::string(argv[1]) == "--public-weather-config") {
        auto config = deskduo::defaultConfig();
        config.weather.url = argv[2];
        config.weather.headers = {{"X-Feed", "header-secret"}};
        std::cout << deskduo::redactedConfig(config);
        return 0;
    }
    CHECK(deskduo::parseDisplayPreference(R"({"brightnessAuto":true,"brightnessDayStart":"07:15","brightnessNightStart":"19:45"})").value);
    for (const auto* bad : {R"({"brightnessAuto":1})",R"({"brightnessAuto":null})",R"({"brightnessAuto":false,"brightnessAuto":true})",R"({"brightnessDayStart":"6:00"})",R"({"brightnessDayStart":"24:00"})",R"({"brightnessNightStart":"00:60"})",R"({"brightnessNightStart":360})",R"({"brightnessDayStart":"06:00x"})",R"({"brightnessPhase":"day"})",R"({"brightnessTargetPercent":5})",R"({"brightnessEffectivePercent":5})"}) CHECK(!deskduo::parseDisplayPreference(bad).value);
    { deskduo::DisplayPreference pref; int writes=0;
      CHECK(pref.patch(R"({"brightnessDayStart":"18:00"})",[&](const auto&){++writes;return true;})==400);
      CHECK(writes==0);
      CHECK(pref.patch(R"({"brightnessAuto":true,"brightnessDayStart":"19:45","brightnessNightStart":"07:15"})",[&](const auto&){++writes;return true;})==204);
      CHECK(pref.settings().brightnessAuto && pref.settings().brightnessDayStart==1185 && pref.settings().brightnessNightStart==435 && writes==1);
      CHECK(pref.patch(R"({"brightnessAuto":false})",[&](const auto&){++writes;return true;})==204 && !pref.settings().brightnessAuto);
    }
    if (argc != 2) {
        std::cerr << "usage: firmware_core_test REPOSITORY_ROOT\n";
        return 2;
    }
    try {
        const std::string root = argv[1];
        // Removing auth role separation, length caps, sticky header stripping, or
        // the absolute deadline must fail these public-behaviour regressions.
        CHECK(deskduo::authorized("Basic YWRtaW46c2VjcmV0", false, "secret", "usage-secret"));
        CHECK(!deskduo::authorized("Bearer usage-secret", false, "secret", "usage-secret"));
        CHECK(deskduo::authorized("Bearer usage-secret", true, "secret", "usage-secret"));
        CHECK(!deskduo::authorized("Basic YWRtaW46c2VjcmV0", true, "secret", "usage-secret"));
        CHECK(!deskduo::authorized("Bearer usage-secreu", true, "secret", "usage-secret"));
        CHECK(!deskduo::authorized("Bearer ", true, "secret", ""));
        CHECK(!deskduo::authorized("", true, "secret", ""));
        CHECK(!deskduo::authorized("Bearer usage-secret", true, "secret", ""));
        CHECK(!deskduo::constantEqual("a", std::string("a\0b", 3)));
        CHECK(deskduo::bodyPolicy(4096, "Application/JSON; charset=utf-8", 4096) == 0);
        CHECK(deskduo::bodyPolicy(4097, "application/json", 4096) == 413);
        CHECK(deskduo::bodyPolicy(1, "text/plain", 8192) == 415);
        CHECK(deskduo::bodyPolicy(0, "application/json", 8192) == 400);
        CHECK(deskduo::bodyPolicy(1, "application/jsonp", 8192) == 415);
        // Every spelling accepted by config/redirect policy must select verified
        // TLS and require a usable clock; a case-sensitive test leaks HTTP bytes.
        for (const std::string& url : {"https://one.test/data", "HTTPS://one.test/data?accessToken=private", "HtTpS://one.test/data"}) {
            auto configured = deskduo::applyConfigPatch(deskduo::defaultConfig(),
                "{\"weather\":{\"url\":\"" + url + "\"}}");
            CHECK(configured.value.has_value());
            CHECK(deskduo::transportPolicy(configured.value->weather.url, 0).secure);
            CHECK(!deskduo::transportPolicy(configured.value->weather.url, 0).mayConnect);
            CHECK(deskduo::transportPolicy(configured.value->weather.url, 1700000000).secure);
            CHECK(deskduo::transportPolicy(configured.value->weather.url, 1700000000).mayConnect);
            deskduo::FetchPolicy redirect("https://one.test/original", 100);
            CHECK(redirect.redirect(url));
            CHECK(redirect.sendHeaders);
            CHECK(deskduo::transportPolicy(redirect.url, 0).secure);
            CHECK(!deskduo::transportPolicy(redirect.url, 0).mayConnect);
            CHECK(deskduo::transportPolicy(redirect.url, 1700000000).mayConnect);
            CHECK(redirect.remaining(10100) == 0);
        }
        CHECK(!deskduo::transportPolicy("ftp://one.test/data", 1700000000).mayConnect);
        CHECK(!deskduo::transportPolicy("https:///data", 1700000000).mayConnect);
        CHECK(!deskduo::transportPolicy("HTTP://one.test/data", 0).secure);
        CHECK(deskduo::transportPolicy("HTTP://one.test/data", 0).mayConnect);
        // Root-relative targets replace the old path/query, including pathless
        // origins whose query itself contains slashes or another complete URL.
        for (const std::string& from : {
            "https://one.test?accessToken=private",
            "https://one.test?next=/old/path",
            "https://one.test?next=https://other.test/path",
            "https://one.test/old/path?accessToken=private",
            "https://one.test"}) {
            CHECK(deskduo::redirectTargetAllowed(from, from));
            CHECK(deskduo::resolveRedirectTarget(from, "/data?new=1") == "https://one.test/data?new=1");
            deskduo::FetchPolicy relative(from, 100);
            CHECK(relative.redirect("/data?new=1"));
            CHECK(relative.url == "https://one.test/data?new=1");
            CHECK(relative.sendHeaders);
            CHECK(relative.remaining(10100) == 0);
        }
        deskduo::FetchPolicy mixedRelative("HtTpS://one.test:443?next=/old", 100);
        CHECK(mixedRelative.redirect("/data"));
        CHECK(mixedRelative.url == "HtTpS://one.test:443/data");
        CHECK(mixedRelative.sendHeaders);
        CHECK(deskduo::transportPolicy(mixedRelative.url, 0).secure);
        CHECK(!deskduo::transportPolicy(mixedRelative.url, 0).mayConnect);
        CHECK(!mixedRelative.redirect("//other.test/data"));
        CHECK(!mixedRelative.redirect("relative/path"));
        CHECK(!mixedRelative.redirect("HTTP://one.test/data"));
        deskduo::FetchPolicy upgrade("http://one.test/start", 100);
        CHECK(upgrade.redirect("HtTpS://one.test/data"));
        CHECK(!upgrade.sendHeaders);
        CHECK(deskduo::transportPolicy(upgrade.url, 1699999999).secure);
        CHECK(!deskduo::transportPolicy(upgrade.url, 1699999999).mayConnect);
        CHECK(deskduo::transportPolicy(upgrade.url, 1700000000).mayConnect);
        CHECK(upgrade.redirect("HTTPS://two.test/data"));
        CHECK(upgrade.redirect("https://one.test/data"));
        CHECK(!upgrade.sendHeaders);
        deskduo::FetchPolicy fetch("https://one.test/start", 100);
        CHECK(fetch.remaining(100) == 10000);
        CHECK(fetch.remaining(10099) == 1);
        CHECK(fetch.remaining(10100) == 0);
        CHECK(fetch.append(16384));
        CHECK(!fetch.append(1));
        CHECK(fetch.redirect("https://one.test/next"));
        CHECK(fetch.sendHeaders);
        CHECK(fetch.redirect("https://two.test/next"));
        CHECK(!fetch.sendHeaders);
        CHECK(fetch.redirect("https://one.test/back"));
        CHECK(!fetch.sendHeaders);
        CHECK(fetch.redirect("https://one.test/four"));
        CHECK(!fetch.redirect("https://one.test/five"));
        deskduo::FetchPolicy downgrade("https://one.test", 0);
        CHECK(!downgrade.redirect("http://one.test"));
        CHECK(downgrade.url == "https://one.test");
        // Sarabun GSUB: narrow i + compact mai-tri; F731 is the tall E34 context.
        CHECK(deskduo::shapeText("ปิ๊") == "ป\uf701\uf731");
        // Source uses yoYingthai.less before a below vowel, retaining the normal base otherwise.
        CHECK(deskduo::shapeText("ญุ") == "\uf71a\uf717");
        CHECK(deskduo::shapeText("ญ") == "ญ");
        CHECK(deskduo::shapeText("ฐู") == "\uf71b\uf718");
        CHECK(deskduo::shapeText("ฐ") == "ฐ");
        // F72A is compact mai-ek above E34; default F70D belongs to the E35 context.
        CHECK(deskduo::shapeText("กิ่") == "กิ\uf72a");
        // Source's E4D+tone ligature already contains the above dot.
        CHECK(deskduo::shapeText("กํ่") == "ก\uf752");
        for (const char* text : {"ปิ๊", "ญุ", "ฐู", "กิ่", "กํ่", "ปํ้", "กํ์"}) {
            const auto shaped = deskduo::shapeText(text);
            CHECK(deskduo::shapeText(shaped) == shaped);
        }
        CHECK(deskduo::shapeText("Claude 32% หุ้น") == "Claude 32% หุ้น");
        CHECK(deskduo::shapeText("☃") == "?");
        CHECK(deskduo::compactNumber(1e300).size() < 16);
        CHECK(deskduo::compactNumber(-1e300).size() < 16);
        CHECK(deskduo::compactNumber(12.5) == "12.50");
        auto intended = deskduo::defaultConfig();
        intended.ssid = "private-wifi";
        intended.wifiPassword = "private-password";
        intended.usageToken = "0123456789abcdef0123456789abcdef";
        CHECK(!deskduo::completeSerialization(intended, "{}"));
        CHECK(!deskduo::completeSerialization(intended, ""));
        CHECK(deskduo::completeSerialization(intended, deskduo::serializeConfig(intended)));
        // Rejecting invalid preferences must never write; failed commit must
        // retain the active view; successful commit must precede publication.
        deskduo::DisplayPreference preference;
        using Mode = deskduo::CodexUsageMode;
        CHECK(preference.get() == Mode::Both);
        int writes=0;
        for(const auto& bad:std::vector<std::string>{"{}", "[]", "null", R"({"codexUsageMode":"5h"})", R"({"codexUsageMode":1})", R"({"codexUsageMode":"weekly","extra":true})", R"({"codexUsageMode":"both","codexUsageMode":"weekly"})", R"({"codexUsageMode":"weekly"}x)", std::string(513,' '), std::string(40,'[')+std::string(40,']')}) {
            CHECK(preference.patch(bad,[&](const deskduo::DisplaySettings&){++writes;return true;}) == 400);
        }
        CHECK(writes == 0);
        CHECK(preference.patch(R"({"brightnessPercent":5})",[&](const deskduo::DisplaySettings&){++writes;return true;}) == 204);

        CHECK(preference.patch(R"({"codexUsageMode":"weekly"})",[&](const deskduo::DisplaySettings& next){++writes;CHECK(next.mode == Mode::Weekly);CHECK(preference.get() == Mode::Both);return false;}) == 503);
        CHECK(preference.get() == Mode::Both);
        CHECK(preference.settings().brightnessPercent == 5);
        CHECK(preference.patch(R"({"codexUsageMode":"weekly"})",[&](const deskduo::DisplaySettings& next){++writes;CHECK(next.mode == Mode::Weekly);CHECK(preference.get() == Mode::Both);return true;}) == 204);
        CHECK(preference.get() == Mode::Weekly);
        CHECK(preference.patch(R"({"codexUsageMode":"both"})",[&](const deskduo::DisplaySettings&){CHECK(preference.get() == Mode::Weekly);return true;}) == 204);
        CHECK(preference.get() == Mode::Both);
        deskduo::Usage selected;
        selected.primary=deskduo::UsageWindow{15,300,4102444800};
        selected.secondary=deskduo::UsageWindow{63,10080,4102531200};
        auto weekly=deskduo::codexWeeklyWindow(selected);
        CHECK(weekly && weekly->usedPercent==63 && weekly->resetsAt==4102531200);
        selected.primary->windowMinutes=10080;CHECK(deskduo::codexWeeklyWindow(selected)->usedPercent==15);
        selected.primary->windowMinutes=300;selected.secondary->windowMinutes.reset();
        CHECK(deskduo::codexWeeklyWindow(selected)->usedPercent==63);
        selected.secondary->windowMinutes=90.5;CHECK(!deskduo::codexWeeklyWindow(selected));
        selected.secondary.reset();CHECK(!deskduo::codexWeeklyWindow(selected));
        selected.primary->windowMinutes.reset();CHECK(deskduo::codexWeeklyWindow(selected)->usedPercent==15);
        selected.secondary=deskduo::UsageWindow{12,300,{}};CHECK(!deskduo::codexWeeklyWindow(selected));
        selected.primary.reset();selected.secondary->windowMinutes=10080;CHECK(deskduo::codexWeeklyWindow(selected)->usedPercent==12);
        selected.secondary.reset();CHECK(!deskduo::codexWeeklyWindow(selected));
        // Direct-stock settings must be accepted and preserved by sparse saves.
        auto direct = deskduo::applyConfigPatch(deskduo::defaultConfig(),
            R"({"stocks":{"provider":"yahoo","watchlist":[{"symbol":"PTT.BK","enabled":true}],"chartRange":"5d"}})");
        CHECK(direct.value.has_value());
        CHECK(deskduo::completeSerialization(*direct.value, deskduo::serializeConfig(*direct.value)));
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
