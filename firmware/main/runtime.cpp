#include "runtime.hpp"
#include "bootloader_random.h"
#include "esp_mac.h"
#include "esp_random.h"
#include "esp_timer.h"
#include "freertos/FreeRTOS.h"
#include "freertos/semphr.h"
#include "nvs.h"
#include "nvs_flash.h"
#include <cstdio>
#include <vector>
namespace deskduo {
namespace {
Config config;
Model model;
DisplayPreference displayPreference;
SemaphoreHandle_t mutex;
SemaphoreHandle_t writer;
class Lock {
    SemaphoreHandle_t value;
public:
    explicit Lock(SemaphoreHandle_t m) : value(m) { xSemaphoreTake(value, portMAX_DELAY); }
    ~Lock() { xSemaphoreGive(value); }
};
std::string randomString(std::size_t count, const char* alphabet) {
    std::string out;
    const auto length = std::char_traits<char>::length(alphabet);
    while (out.size() < count) {
        uint8_t byte; esp_fill_random(&byte, 1);
        if (byte < 256 - 256 % length) out += alphabet[byte % length];
    }
    return out;
}
bool persist(const std::string& json) {
    if (json.empty()) return false;
    nvs_handle_t handle;
    if (nvs_open("deskduo", NVS_READWRITE, &handle) != ESP_OK) return false;
    const bool ok = nvs_set_blob(handle, "config", json.data(), json.size()) == ESP_OK && nvs_commit(handle) == ESP_OK;
    nvs_close(handle);
    return ok;
}
}
uint64_t now_ms() { return esp_timer_get_time() / 1000; }
void runtime_init() {
    // Do not erase NVS on a failed init or malformed saved configuration.
    ESP_ERROR_CHECK(nvs_flash_init());
    mutex = xSemaphoreCreateMutex(); writer = xSemaphoreCreateMutex();
    assert(mutex && writer);
    config = defaultConfig();
    nvs_handle_t handle;
    ESP_ERROR_CHECK(nvs_open("deskduo", NVS_READWRITE, &handle));
    std::size_t length = 0;
    esp_err_t result = nvs_get_blob(handle, "config", nullptr, &length);
    if (result == ESP_OK) {
        ESP_ERROR_CHECK(length == 0 || length > 32768 ? ESP_ERR_INVALID_SIZE : ESP_OK);
        std::vector<char> data(length);
        ESP_ERROR_CHECK(nvs_get_blob(handle, "config", data.data(), &length));
        auto saved = parseStoredConfig(std::string(data.begin(), data.end()));
        ESP_ERROR_CHECK(saved.value ? ESP_OK : ESP_ERR_INVALID_STATE);
        config = std::move(*saved.value);
    } else ESP_ERROR_CHECK(result == ESP_ERR_NVS_NOT_FOUND ? ESP_OK : result);
    // Setters write immediately on the pinned IDF; commit is not a multi-key
    // transaction. The wider authoritative scalar is never split across keys.
    auto preferenceRead = [](esp_err_t error) {
        ESP_ERROR_CHECK(error == ESP_ERR_NVS_NOT_FOUND || error == ESP_ERR_NVS_TYPE_MISMATCH ? ESP_OK : error);
    };
    uint32_t savedV3 = 0;
    result = nvs_get_u32(handle, "ui_pref_v3", &savedV3);
    preferenceRead(result);
    const auto decoded = decodeDisplaySettings(savedV3);
    if (result == ESP_OK && validDisplaySettings(decoded)) displayPreference.initialize(decoded);
    else {
        uint16_t savedV2 = 0;
        result = nvs_get_u16(handle, "ui_pref_v2", &savedV2);
        preferenceRead(result);
        const auto percent = savedV2 >> 2;
        if (result == ESP_OK && percent >= 5 && percent <= 100)
            displayPreference.initialize(static_cast<CodexUsageMode>(savedV2 & 1),
                static_cast<Language>((savedV2 >> 1) & 1), static_cast<uint8_t>(percent));
        else {
            uint8_t old = 0;
            result = nvs_get_u8(handle, "ui_pref", &old);
            preferenceRead(result);
            if (result == ESP_OK && old <= 3)
                displayPreference.initialize(static_cast<CodexUsageMode>(old & 1), static_cast<Language>((old >> 1) & 1));
            else {
                uint8_t legacy = 0;
                result = nvs_get_u8(handle, "codex_layout", &legacy);
                preferenceRead(result);
                displayPreference.initialize(result == ESP_OK && legacy <= 1 ? static_cast<CodexUsageMode>(legacy) : CodexUsageMode::Both);
            }
        }
    }
    nvs_close(handle);
    bool changed = false;
    // Enable the documented internal entropy source before Wi-Fi starts.
    // Disable it before RF/ADC use; esp_fill_random supplies independent secrets.
    bootloader_random_enable();
    if (config.setupPassword.empty()) {
        config.setupPassword = randomString(12, "ABCDEFGHJKLMNPQRSTUVWXYZabcdefghijkmnopqrstuvwxyz23456789"); changed = true;
    }
    if (config.usageToken.empty()) {
        config.usageToken = randomString(32, "0123456789abcdef"); changed = true;
    }
    bootloader_random_disable();
    if (changed) {
        auto encoded = serializeConfig(config);
        ESP_ERROR_CHECK(completeSerialization(config, encoded) && persist(encoded) ? ESP_OK : ESP_FAIL);
    }
    model.applyConfig(config);
}
RequestConfig runtime_config() { Lock lock(mutex); return {config, model.generation()}; }
Snapshot runtime_snapshot() { return model.snapshot(now_ms()); }
WeatherView runtime_weather_view() {
    Lock lock(mutex);
    return {model.generation(), !config.weather.url.empty(), model.snapshot(now_ms()).weather};
}
int runtime_patch(const std::string& json) {
    Lock oneWriter(writer);
    Config before = runtime_config().config;
    auto parsed = applyConfigPatch(before, json);
    if (!parsed.value) return 400;
    auto encoded = serializeConfig(*parsed.value);
    if (!completeSerialization(*parsed.value, encoded) || !persist(encoded)) return 503;
    { Lock lock(mutex); config = std::move(*parsed.value); model.applyConfig(config); }
    return 204;
}
DisplaySettings runtime_display_settings() { return displayPreference.settings(); }
Language runtime_language() { return displayPreference.language(); }
CodexUsageMode runtime_display_mode() { return displayPreference.get(); }
int runtime_display_patch(const std::string& json) {
    Lock oneWriter(writer);
    return displayPreference.patch(json, [](const DisplaySettings& next) {
        nvs_handle_t handle;
        if (nvs_open("deskduo", NVS_READWRITE, &handle) != ESP_OK) return false;
        const uint32_t encoded = encodeDisplaySettings(next);
        const bool ok = nvs_set_u32(handle, "ui_pref_v3", encoded) == ESP_OK && nvs_commit(handle) == ESP_OK;
        nvs_close(handle);
        return ok;
    });
}
void runtime_usage(const Usage& usage) { model.acceptUsage(usage, now_ms()); }
void runtime_weather(const ParseResult<Weather>& value, uint64_t generation) {
    if (value.value) model.acceptWeather(*value.value, now_ms(), generation);
    else model.markFailed("weather", generation);
}
void runtime_stocks(const ParseResult<Stocks>& value, uint64_t generation) {
    if (value.value) model.acceptStocks(*value.value, now_ms(), generation);
    else model.markFailed("stocks", generation);
}
std::string ap_name() {
    uint8_t mac[6]; ESP_ERROR_CHECK(esp_read_mac(mac, ESP_MAC_WIFI_SOFTAP));
    char name[24]; std::snprintf(name, sizeof(name), "DeskDuo-%02X%02X%02X", mac[3], mac[4], mac[5]);
    return name;
}
}
