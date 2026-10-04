#pragma once
// Host-side SDK boundary only. Production routes and worker run unchanged.
#include <cstddef>
#include <cstdint>
#include <cassert>
#include <map>
#include <string>
using esp_err_t=int;
constexpr int ESP_OK=0,ESP_FAIL=-1,HTTPD_SOCK_ERR_TIMEOUT=-3,pdPASS=1;
#define ESP_ERROR_CHECK(value) assert((value)==ESP_OK)
using httpd_handle_t=void*;
enum httpd_method_t {HTTP_GET,HTTP_POST};
struct Exchange {std::map<std::string,std::string> headers;std::string body,status,response;size_t offset=0;int reads=0;};
struct httpd_req_t {httpd_handle_t handle=nullptr;httpd_method_t method=HTTP_POST;const char* uri="/api/firmware";size_t content_len=0;Exchange* exchange=nullptr;};
using Handler=esp_err_t(*)(httpd_req_t*);
struct httpd_uri_t {const char* uri;httpd_method_t method;Handler handler;};
struct httpd_config_t {int stack_size,max_open_sockets;bool lru_purge_enable;int max_uri_handlers,recv_wait_timeout,send_wait_timeout;};
#define HTTPD_DEFAULT_CONFIG() httpd_config_t{}
size_t httpd_req_get_hdr_value_len(httpd_req_t*,const char*);
int httpd_req_get_hdr_value_str(httpd_req_t*,const char*,char*,size_t);
int httpd_req_recv(httpd_req_t*,char*,size_t);
int httpd_resp_set_status(httpd_req_t*,const char*);
int httpd_resp_set_type(httpd_req_t*,const char*);
int httpd_resp_set_hdr(httpd_req_t*,const char*,const char*);
int httpd_resp_send(httpd_req_t*,const char*,size_t);
int httpd_resp_sendstr(httpd_req_t*,const char*);
int httpd_req_async_handler_begin(httpd_req_t*,httpd_req_t**);
int httpd_req_async_handler_complete(httpd_req_t*);
int httpd_req_to_sockfd(httpd_req_t*);
int httpd_sess_trigger_close(httpd_handle_t,int);
int httpd_start(httpd_handle_t*,const httpd_config_t*);
int httpd_register_uri_handler(httpd_handle_t,const httpd_uri_t*);
struct esp_partition_t {size_t size;const char* label;};
using esp_ota_handle_t=unsigned;
enum esp_ota_img_states_t {ESP_OTA_IMG_NEW,ESP_OTA_IMG_PENDING_VERIFY,ESP_OTA_IMG_VALID,ESP_OTA_IMG_UNDEFINED};
const esp_partition_t* esp_ota_get_next_update_partition(const esp_partition_t*);
const esp_partition_t* esp_ota_get_running_partition();
int esp_ota_get_state_partition(const esp_partition_t*,esp_ota_img_states_t*);
int esp_ota_begin(const esp_partition_t*,size_t,esp_ota_handle_t*);
int esp_ota_write(esp_ota_handle_t,const void*,size_t);
int esp_ota_end(esp_ota_handle_t);
int esp_ota_abort(esp_ota_handle_t);
int esp_ota_set_boot_partition(const esp_partition_t*);
int esp_ota_mark_app_valid_cancel_rollback();
struct esp_app_desc_t {const char* version;};
const esp_app_desc_t* esp_app_get_description();
using TaskFunction=void(*)(void*);
int xTaskCreate(TaskFunction,const char*,int,void*,int,void*);
void vTaskDelete(void*);
void vTaskDelay(int);
void esp_restart();
#define pdMS_TO_TICKS(value) (value)
