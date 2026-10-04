// Hardware test only. Never initializes NVS, radio, configuration, or the UI.
// A pending trial boot must roll back when this resets without confirmation.
#include "esp_system.h"
#include "freertos/FreeRTOS.h"
#include "freertos/task.h"
void app_main(void) {
    vTaskDelay(pdMS_TO_TICKS(500));
    esp_restart();
}
