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
