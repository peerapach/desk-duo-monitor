#include "input.hpp"
#include "runtime.hpp"
#include "driver/gpio.h"
#include "driver/spi_master.h"
#include "lvgl.h"
#include <algorithm>
namespace deskduo {
namespace {
spi_device_handle_t touch;
int readRaw(uint8_t command) {
    uint8_t tx[3] = {command,0,0}, rx[3]{};
    spi_transaction_t t{}; t.length = 24; t.rxlength = 24; t.tx_buffer = tx; t.rx_buffer = rx;
    if (spi_device_polling_transmit(touch, &t) != ESP_OK) return -1;
    return ((rx[1] << 8 | rx[2]) >> 3) & 4095;
}
int median(uint8_t cmd) { int values[] = {readRaw(cmd),readRaw(cmd),readRaw(cmd)}; std::sort(values,values+3); return values[1]; }
}
bool input_read(int& x, int& y) {
    const int z = median(0xB0);
    if (z < 100 || z > 3000) return false;
    auto point = mapTouch(median(0xD0), median(0x90), runtime_config().config.touch);
    x = point.first; y = point.second; return true;
}
void input_init() {
    spi_bus_config_t bus{}; bus.mosi_io_num = 32; bus.miso_io_num = 39; bus.sclk_io_num = 25;
    bus.quadwp_io_num = -1; bus.quadhd_io_num = -1; bus.max_transfer_sz = 32;
    ESP_ERROR_CHECK(spi_bus_initialize(SPI3_HOST, &bus, SPI_DMA_DISABLED));
    spi_device_interface_config_t dev{}; dev.clock_speed_hz = 1000000; dev.spics_io_num = 33; dev.queue_size = 1;
    ESP_ERROR_CHECK(spi_bus_add_device(SPI3_HOST, &dev, &touch));
    gpio_config_t pins{}; pins.pin_bit_mask = (1ULL << 36) | 1; pins.mode = GPIO_MODE_INPUT;
    ESP_ERROR_CHECK(gpio_config(&pins)); gpio_set_pull_mode(GPIO_NUM_0, GPIO_PULLUP_ONLY);
    auto* input = lv_indev_create(); lv_indev_set_type(input,LV_INDEV_TYPE_POINTER);
    lv_indev_set_read_cb(input, [](lv_indev_t*, lv_indev_data_t* d) {
        static int x = 0, y = 0; static unsigned pressed = 0;
        bool down = input_read(x,y); pressed = down ? pressed + 1 : 0;
        d->point.x = x; d->point.y = y;
        d->state = pressed >= 2 ? LV_INDEV_STATE_PRESSED : LV_INDEV_STATE_RELEASED;
    });
}
}
