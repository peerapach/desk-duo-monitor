#include "runtime.hpp"
#include "brightness_policy.hpp"
#include "network.hpp"
#include "portal.hpp"
#include "display.hpp"
#include "input.hpp"
#include "ui.hpp"
#include "ota.hpp"
#include "esp_log.h"
#include "freertos/FreeRTOS.h"
#include "freertos/task.h"
#include "driver/gpio.h"
#include "lvgl.h"
#include <cstdlib>
#include <ctime>
extern "C" void app_main() {
    // SDK transport diagnostics can include configured hosts/URLs: suppress them.
    esp_log_level_set("*", ESP_LOG_NONE);
    setenv("TZ", "ICT-7", 1); tzset();
    deskduo::runtime_init();
    lv_init();
    lv_tick_set_cb([]() -> uint32_t { return deskduo::now_ms(); });
    deskduo::display_init(deskduo::brightness_plan(deskduo::runtime_display_settings(), std::time(nullptr)).percent); deskduo::input_init(); deskduo::ui_init();
    deskduo::network_init(); deskduo::portal_init();
    ESP_ERROR_CHECK(xTaskCreate(deskduo::network_task, "sources", 12288, nullptr, 4, nullptr) == pdPASS ? ESP_OK : ESP_ERR_NO_MEM);
    // This is the only task allowed to call LVGL, including timer_handler.
    bool confirmed = false;
    for (;;) {
        deskduo::ui_tick(); lv_timer_handler();
        if (!confirmed) { deskduo::ota_confirm_boot(); confirmed = true; }
        vTaskDelay(pdMS_TO_TICKS(20));
    }
}
