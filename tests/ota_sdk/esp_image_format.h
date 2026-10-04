#pragma once
#include "esp_http_server.h"
struct esp_partition_pos_t {uint32_t offset;uint32_t size;};
struct esp_image_metadata_t {uint32_t image_len;};
enum esp_image_load_mode_t {ESP_IMAGE_VERIFY};
esp_err_t esp_image_verify(esp_image_load_mode_t,const esp_partition_pos_t*,esp_image_metadata_t*);
