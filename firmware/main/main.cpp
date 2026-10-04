#include "runtime.hpp"
#include "network.hpp"
#include "portal.hpp"
#include "display.hpp"
#include "input.hpp"
#include "ui.hpp"
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
    deskduo::display_init(); deskduo::input_init(); deskduo::ui_init();
    deskduo::network_init(); deskduo::portal_init();
    assert(xTaskCreate(deskduo::network_task, "sources", 12288, nullptr, 4, nullptr) == pdPASS);
    // This is the only task allowed to call LVGL, including timer_handler.
    for (;;) { deskduo::ui_tick(); lv_timer_handler(); vTaskDelay(pdMS_TO_TICKS(20)); }
}
