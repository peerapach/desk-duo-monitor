#include "backlight.hpp"
#include "driver/ledc.h"
#include <atomic>
namespace deskduo {
namespace {
bool initialized = false;
int appliedDuty = -1;
std::atomic<uint8_t> appliedPercent{0};
bool valid(uint8_t percent) { return percent >= 5 && percent <= 100; }
uint32_t duty(uint8_t percent) { return (static_cast<uint32_t>(percent) * 4095 + 50) / 100; }
}
std::optional<uint8_t> backlight_last_applied_percent() {
    const auto percent = appliedPercent.load();
    return percent ? std::optional<uint8_t>(percent) : std::nullopt;
}
esp_err_t backlight_init(uint8_t brightnessPercent) {
    if (!valid(brightnessPercent)) return ESP_ERR_INVALID_ARG;
    initialized = false;
    appliedDuty = -1;
    ledc_timer_config_t timer{};
    timer.speed_mode = LEDC_LOW_SPEED_MODE;
    timer.duty_resolution = LEDC_TIMER_12_BIT;
    timer.timer_num = LEDC_TIMER_0;
    timer.freq_hz = 5000;
    timer.clk_cfg = LEDC_AUTO_CLK;
    auto error = ledc_timer_config(&timer);
    if (error != ESP_OK) return error;
    ledc_channel_config_t channel{};
    channel.gpio_num = 21;
    channel.speed_mode = LEDC_LOW_SPEED_MODE;
    channel.channel = LEDC_CHANNEL_0;
    channel.intr_type = LEDC_INTR_DISABLE;
    channel.timer_sel = LEDC_TIMER_0;
    channel.duty = duty(brightnessPercent);
    channel.hpoint = 0;
    error = ledc_channel_config(&channel);
    if (error != ESP_OK) return error;
    initialized = true;
    appliedDuty = static_cast<int>(channel.duty);
    appliedPercent.store(brightnessPercent);
    return ESP_OK;
}
esp_err_t display_set_brightness(uint8_t brightnessPercent) {
    if (!valid(brightnessPercent)) return ESP_ERR_INVALID_ARG;
    if (!initialized) return ESP_ERR_INVALID_STATE;
    const auto next = duty(brightnessPercent);
    if (appliedDuty == static_cast<int>(next)) return ESP_OK;
    auto error = ledc_set_duty(LEDC_LOW_SPEED_MODE, LEDC_CHANNEL_0, next);
    if (error != ESP_OK) return error;
    error = ledc_update_duty(LEDC_LOW_SPEED_MODE, LEDC_CHANNEL_0);
    if (error == ESP_OK) { appliedDuty = static_cast<int>(next); appliedPercent.store(brightnessPercent); }
    return error;
}
}
