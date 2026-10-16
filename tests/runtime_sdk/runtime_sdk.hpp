#pragma once
#include <cassert>
#include <cstddef>
#include <cstdint>
using esp_err_t=int;
using nvs_handle_t=int;
using SemaphoreHandle_t=int;
constexpr int ESP_OK=0, ESP_FAIL=-1, ESP_ERR_INVALID_SIZE=-2, ESP_ERR_INVALID_STATE=-3, ESP_ERR_NVS_NOT_FOUND=-4, ESP_ERR_NVS_TYPE_MISMATCH=-5;
constexpr int NVS_READWRITE=1, portMAX_DELAY=0, ESP_MAC_WIFI_SOFTAP=0;
// A test-boundary exception exposes the error passed to the SDK fatal check
// without terminating the entire native test process.
inline void runtime_sdk_error_check(int code){if(code!=ESP_OK)throw code;}
#define ESP_ERROR_CHECK(expr) runtime_sdk_error_check(expr)
int xSemaphoreCreateMutex();
int xSemaphoreTake(int,int);
int xSemaphoreGive(int);
int nvs_flash_init();
int nvs_open(const char*,int,nvs_handle_t*);
int nvs_get_blob(nvs_handle_t,const char*,void*,std::size_t*);
int nvs_set_blob(nvs_handle_t,const char*,const void*,std::size_t);
int nvs_get_u8(nvs_handle_t,const char*,uint8_t*);
int nvs_set_u8(nvs_handle_t,const char*,uint8_t);
int nvs_get_u16(nvs_handle_t,const char*,uint16_t*);
int nvs_set_u16(nvs_handle_t,const char*,uint16_t);
int nvs_get_u32(nvs_handle_t,const char*,uint32_t*);
int nvs_set_u32(nvs_handle_t,const char*,uint32_t);
int nvs_commit(nvs_handle_t);
void nvs_close(nvs_handle_t);
void bootloader_random_enable();
void bootloader_random_disable();
void esp_fill_random(void*,std::size_t);
int esp_read_mac(uint8_t*,int);
int64_t esp_timer_get_time();
