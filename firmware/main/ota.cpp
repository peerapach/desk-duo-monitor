#include "portal.hpp"
#include "ota.hpp"
#include "ota_policy.hpp"
#include "runtime.hpp"
#include "esp_app_desc.h"
#include "cJSON.h"
#include "lwip/sockets.h"
#include "esp_ota_ops.h"
#include "esp_system.h"
#include "freertos/FreeRTOS.h"
#include "freertos/task.h"
#include <new>
namespace deskduo {
namespace {
std::string header(httpd_req_t* r,const char* name,size_t cap) {
    size_t n=httpd_req_get_hdr_value_len(r,name);
    if(!n)return {};
    if(n>cap)return "!invalid!"; // Present invalid Origin must never become absent.
    std::string s(n+1,'\0');
    if(httpd_req_get_hdr_value_str(r,name,s.data(),s.size())!=ESP_OK)return "!invalid!";
    s.resize(n);return s;
}
esp_err_t response(httpd_req_t* r,int code) {
    const char* status="500 Internal Server Error";
    switch(code) {case 200:status="200 OK";break;case 400:status="400 Bad Request";break;case 401:status="401 Unauthorized";break;case 403:status="403 Forbidden";break;case 408:status="408 Request Timeout";break;case 413:status="413 Content Too Large";break;case 415:status="415 Unsupported Media Type";break;case 503:status="503 Service Unavailable";break;}
    httpd_resp_set_status(r,status);
    httpd_resp_set_type(r,"application/json");
    httpd_resp_set_hdr(r,"Cache-Control","no-store");
    httpd_resp_set_hdr(r,"Connection","close");
    return httpd_resp_sendstr(r,code==200?"{\"state\":\"restarting\"}":"{\"error\":\"Update rejected or interrupted; retry with an application image\"}");
}
struct Upload {
    httpd_req_t* request;
    const esp_partition_t* target;
    esp_ota_handle_t handle=0;
    uint64_t now() { return now_ms(); }
    int receive(unsigned char* out,size_t size) { int n=httpd_req_recv(request,reinterpret_cast<char*>(out),size);return n==HTTPD_SOCK_ERR_TIMEOUT?-2:n; }
    bool begin(size_t bytes) {
        handle=0;
        if(esp_ota_begin(target,bytes,&handle)==ESP_OK)return true;
        // IDF 5.5 may allocate the handle before an erase failure. Unlike end(),
        // that failure path does not release the SDK transaction itself.
        if(handle) {esp_ota_abort(handle);handle=0;}
        return false;
    }
    bool write(const unsigned char* p,size_t n) { return esp_ota_write(handle,p,n)==ESP_OK; }
    bool end() { return esp_ota_end(handle)==ESP_OK; }
    void abort() { esp_ota_abort(handle); }
    bool select() { return esp_ota_set_boot_partition(target)==ESP_OK; }
};
void worker(void* arg) {
    auto* upload=static_cast<Upload*>(arg);
    int result=stream_update(*upload,upload->request->content_len);
    response(upload->request,result);
    const int socket=httpd_req_to_sockfd(upload->request);
    const auto server=upload->request->handle;
    // Shut down before returning async ownership so an unread body cannot be
    // parsed as another request. Keep FD ownership with HTTPD.
    shutdown(socket, SHUT_RD);
    httpd_req_async_handler_complete(upload->request);
    httpd_sess_trigger_close(server,socket);
    delete upload;
    if(result==200) { vTaskDelay(pdMS_TO_TICKS(1200));esp_restart(); }
    maintenance.release();
    vTaskDelete(nullptr);
}
}
bool ota_busy() { return maintenance.updating(); }
std::string ota_status_json() {
    const auto* target=esp_ota_get_next_update_partition(nullptr);
    const auto* running=esp_ota_get_running_partition();
    esp_ota_img_states_t state=ESP_OTA_IMG_UNDEFINED;
    esp_ota_get_state_partition(running,&state);
    auto* json=cJSON_CreateObject();
    if(!json)return "{}";
    cJSON_AddStringToObject(json,"version",esp_app_get_description()->version);
    cJSON_AddStringToObject(json,"runningPartition",running->label);
    cJSON_AddNumberToObject(json,"bootState",state);
    cJSON_AddNumberToObject(json,"capacity",target?target->size:0);
    cJSON_AddBoolToObject(json,"busy",ota_busy());
    char* encoded=cJSON_PrintUnformatted(json);
    std::string result=encoded?encoded:"{}";
    cJSON_free(encoded);cJSON_Delete(json);return result;
}
esp_err_t ota_upload(httpd_req_t* request) {
    const auto config=runtime_config().config;
    const auto* target=esp_ota_get_next_update_partition(nullptr);
    const bool admin=authorized(header(request,"Authorization",256),false,config.setupPassword,config.usageToken);
    int policy=upload_policy(admin,header(request,"X-DeskDuo-Update",32),header(request,"Content-Type",128),request->content_len,target?target->size:0,header(request,"Origin",256),header(request,"Host",256));
    if(!header(request,"Transfer-Encoding",128).empty())policy=400;
    if(policy) {response(request,policy);return ESP_FAIL;}
    if(!target || !maintenance.acquire(Maintenance::Update)) {response(request,503);return ESP_FAIL;}
    auto* upload=new(std::nothrow) Upload{};
    if(!upload) {maintenance.release();response(request,503);return ESP_FAIL;}
    upload->target=target;
    if(httpd_req_async_handler_begin(request,&upload->request)!=ESP_OK) {delete upload;maintenance.release();response(request,503);return ESP_FAIL;}
    if(xTaskCreate(worker,"firmware",8192,upload,3,nullptr)!=pdPASS) {
        response(upload->request,503);
        httpd_req_async_handler_complete(upload->request);
        delete upload;maintenance.release();
        return ESP_FAIL;
    }
    return ESP_OK;
}
void ota_confirm_boot() {
    esp_ota_img_states_t state;
    if(esp_ota_get_state_partition(esp_ota_get_running_partition(),&state)==ESP_OK && state==ESP_OTA_IMG_PENDING_VERIFY)
        ESP_ERROR_CHECK(esp_ota_mark_app_valid_cancel_rollback());
}
}
