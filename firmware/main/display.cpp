// ILI9341 initialization adapted from tamaclaude ct_lcd.c (MIT).
// Copyright (c) 2026 Uthai Moolpak. See THIRD_PARTY_NOTICES.md.
#include "display.hpp"
#include "driver/gpio.h"
#include "driver/spi_master.h"
#include "esp_attr.h"
#include "freertos/FreeRTOS.h"
#include "freertos/task.h"
#include "lvgl.h"
#include <initializer_list>
namespace deskduo {
namespace {
spi_device_handle_t lcd;
DMA_ATTR uint8_t pixels[240 * 24 * 2]; // The only display pixel buffer; no PSRAM.
void write(const void* data, std::size_t length) {
    spi_transaction_t t{}; t.length = length * 8; t.tx_buffer = data;
    ESP_ERROR_CHECK(spi_device_polling_transmit(lcd, &t));
}
void command(uint8_t cmd, std::initializer_list<uint8_t> data = {}) {
    gpio_set_level(GPIO_NUM_15, 0); gpio_set_level(GPIO_NUM_2, 0); write(&cmd, 1);
    if (data.size()) { gpio_set_level(GPIO_NUM_2, 1); write(data.begin(), data.size()); }
    gpio_set_level(GPIO_NUM_15, 1);
}
}
void display_flush(int x1, int y1, int x2, int y2, uint8_t* data) {
    command(0x2A, {uint8_t(x1 >> 8), uint8_t(x1), uint8_t(x2 >> 8), uint8_t(x2)});
    command(0x2B, {uint8_t(y1 >> 8), uint8_t(y1), uint8_t(y2 >> 8), uint8_t(y2)});
    command(0x2C);
    auto count = (x2 - x1 + 1) * (y2 - y1 + 1);
    lv_draw_sw_rgb565_swap(data, count);
    gpio_set_level(GPIO_NUM_15, 0); gpio_set_level(GPIO_NUM_2, 1);
    write(data, count * 2); gpio_set_level(GPIO_NUM_15, 1);
}
void display_init(uint8_t brightnessPercent) {
    gpio_config_t pins{}; pins.pin_bit_mask = (1ULL << 15) | (1ULL << 2) | (1ULL << 21); pins.mode = GPIO_MODE_OUTPUT;
    ESP_ERROR_CHECK(gpio_config(&pins)); gpio_set_level(GPIO_NUM_15, 1); gpio_set_level(GPIO_NUM_21, 0);
    spi_bus_config_t bus{};
    bus.mosi_io_num = 13; bus.miso_io_num = 12; bus.sclk_io_num = 14;
    bus.quadwp_io_num = -1; bus.quadhd_io_num = -1; bus.max_transfer_sz = sizeof(pixels) + 64;
    ESP_ERROR_CHECK(spi_bus_initialize(SPI2_HOST, &bus, SPI_DMA_CH_AUTO));
    spi_device_interface_config_t dev{};
    dev.clock_speed_hz = 40000000; dev.spics_io_num = -1; dev.queue_size = 1;
    ESP_ERROR_CHECK(spi_bus_add_device(SPI2_HOST, &dev, &lcd));
    command(0x01); vTaskDelay(pdMS_TO_TICKS(150)); command(0x11); vTaskDelay(pdMS_TO_TICKS(150));
    command(0xCF,{0x00,0xC1,0x30}); command(0xED,{0x64,0x03,0x12,0x81});
    command(0xE8,{0x85,0x00,0x78}); command(0xCB,{0x39,0x2C,0x00,0x34,0x02});
    command(0xF7,{0x20}); command(0xEA,{0x00,0x00}); command(0xC0,{0x23}); command(0xC1,{0x10});
    command(0xC5,{0x3E,0x28}); command(0xC7,{0x86});
    command(0x36,{0x48}); // Portrait MX|BGR candidate: physical corner/color check still required.
    command(0x3A,{0x55}); command(0xB1,{0x00,0x18}); command(0xB6,{0x08,0x82,0x27});
    command(0xF2,{0x00}); command(0x26,{0x01});
    command(0xE0,{0x0F,0x31,0x2B,0x0C,0x0E,0x08,0x4E,0xF1,0x37,0x07,0x10,0x03,0x0E,0x09,0x00});
    command(0xE1,{0x00,0x0E,0x14,0x03,0x11,0x07,0x31,0xC1,0x48,0x08,0x0F,0x0C,0x31,0x36,0x0F});
    command(0x20); command(0x11); vTaskDelay(pdMS_TO_TICKS(120)); command(0x29);
    auto* display = lv_display_create(240,320);
    lv_display_set_color_format(display, LV_COLOR_FORMAT_RGB565);
    lv_display_set_buffers(display, pixels, nullptr, sizeof(pixels), LV_DISPLAY_RENDER_MODE_PARTIAL);
    lv_display_set_flush_cb(display, [](lv_display_t* d, const lv_area_t* area, uint8_t* data) {
        display_flush(area->x1,area->y1,area->x2,area->y2,data); lv_display_flush_ready(d);
    });
    ESP_ERROR_CHECK(backlight_init(brightnessPercent));
}
}
