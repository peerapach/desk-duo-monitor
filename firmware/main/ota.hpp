#pragma once
#include <string>
#include "esp_http_server.h"
namespace deskduo {
bool ota_busy();
std::string ota_status_json();
esp_err_t ota_upload(httpd_req_t* request);
void ota_confirm_boot();
}
