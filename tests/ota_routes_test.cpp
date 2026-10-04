#include "../firmware/main/ota.hpp"
#include "../firmware/main/ota_policy.hpp"
#include "../firmware/main/portal.hpp"
#include "../firmware/main/runtime.hpp"
#include <cstring>
#include <iostream>
#include <vector>
namespace {
std::map<std::pair<std::string,int>,Handler> routes;
TaskFunction task=nullptr;void* taskArg=nullptr;
int asyncCount=0,completed=0,selected=0,aborted=0,confirmed=0,patches=0,usage=0;
bool asyncFail=false,taskFail=false,endFail=false,beginFail=false;
uint64_t clockMs=0;
esp_ota_img_states_t bootState=ESP_OTA_IMG_PENDING_VERIFY;
esp_partition_t target{0x1e0000,"ota_1"},running{0x1e0000,"ota_0"};
struct TaskExit{};struct Reboot{};
std::vector<unsigned char> flash;
Exchange exchange() { Exchange e;e.headers={{"Authorization",deskduo::basicCredential("test-admin")},{"Content-Type","application/octet-stream"},{"X-DeskDuo-Update","1"},{"Host","192.0.2.10"}};e.body=std::string(4096,'\0');e.body[0]=char(0xe9);e.body[1]=1;e.body[29]=1;e.body[32]=0x32;e.body[33]=0x54;e.body[34]=char(0xcd);e.body[35]=char(0xab);std::memcpy(e.body.data()+80,"desk_duo",9);return e; }
httpd_req_t request(Exchange& e,const char* path="/api/firmware",httpd_method_t method=HTTP_POST) {return {nullptr,method,path,e.body.size(),&e};}
int dispatch(httpd_req_t& r) {return routes.at({r.uri,r.method})(&r);}
void runWorker() {assert(task);auto f=task;task=nullptr;try{f(taskArg);}catch(const TaskExit&){} }
}
size_t httpd_req_get_hdr_value_len(httpd_req_t* r,const char* name){return r->exchange->headers[name].size();}
int httpd_req_get_hdr_value_str(httpd_req_t* r,const char* name,char* out,size_t n){auto& s=r->exchange->headers[name];if(n<=s.size())return ESP_FAIL;std::memcpy(out,s.c_str(),s.size()+1);return ESP_OK;}
int httpd_req_recv(httpd_req_t* r,char* out,size_t n){auto& e=*r->exchange;++e.reads;n=std::min({n,size_t(257),e.body.size()-e.offset});std::memcpy(out,e.body.data()+e.offset,n);e.offset+=n;++clockMs;return n;}
int httpd_resp_set_status(httpd_req_t* r,const char* s){r->exchange->status=s;return ESP_OK;}
int httpd_resp_set_type(httpd_req_t*,const char*){return ESP_OK;}
int httpd_resp_set_hdr(httpd_req_t*,const char*,const char*){return ESP_OK;}
int httpd_resp_send(httpd_req_t* r,const char* s,size_t n){r->exchange->response.assign(s,n);return ESP_OK;}
int httpd_resp_sendstr(httpd_req_t* r,const char* s){return httpd_resp_send(r,s,std::strlen(s));}
int httpd_req_async_handler_begin(httpd_req_t* r,httpd_req_t** out){if(asyncFail)return ESP_FAIL;*out=new httpd_req_t(*r);++asyncCount;return ESP_OK;}
int httpd_req_async_handler_complete(httpd_req_t* r){delete r;++completed;return ESP_OK;}
int httpd_req_to_sockfd(httpd_req_t*){return -1;}
int httpd_sess_trigger_close(httpd_handle_t,int){return ESP_OK;}
int httpd_start(httpd_handle_t* out,const httpd_config_t* c){assert(!c->lru_purge_enable && c->max_open_sockets>=4);*out=nullptr;return ESP_OK;}
int httpd_register_uri_handler(httpd_handle_t,const httpd_uri_t* r){routes[{r->uri,r->method}]=r->handler;return ESP_OK;}
const esp_partition_t* esp_ota_get_next_update_partition(const esp_partition_t*){return &target;}
const esp_partition_t* esp_ota_get_running_partition(){return &running;}
int esp_ota_get_state_partition(const esp_partition_t*,esp_ota_img_states_t* state){*state=bootState;return ESP_OK;}
int esp_ota_begin(const esp_partition_t* p,size_t n,esp_ota_handle_t* handle){assert(p==&target && n==4096);*handle=123;flash.clear();return beginFail?ESP_FAIL:ESP_OK;}
int esp_ota_write(esp_ota_handle_t h,const void* data,size_t n){assert(h==123);auto* p=static_cast<const unsigned char*>(data);flash.insert(flash.end(),p,p+n);return ESP_OK;}
int esp_ota_end(esp_ota_handle_t h){assert(h==123);return endFail?ESP_FAIL:ESP_OK;}
int esp_ota_abort(esp_ota_handle_t h){assert(h==123);++aborted;return ESP_OK;}
int esp_ota_set_boot_partition(const esp_partition_t* p){assert(p==&target);++selected;return ESP_OK;}
int esp_ota_mark_app_valid_cancel_rollback(){++confirmed;return ESP_OK;}
const esp_app_desc_t* esp_app_get_description(){static esp_app_desc_t desc{"2.0.0"};return &desc;}
int xTaskCreate(TaskFunction f,const char*,int,void* p,int,void*){if(taskFail)return 0;task=f;taskArg=p;return pdPASS;}
void vTaskDelete(void*){throw TaskExit{};}
void vTaskDelay(int ticks){clockMs+=ticks;}
void esp_restart(){throw Reboot{};}
namespace deskduo {
extern const char portal_html[]="test page";
RequestConfig runtime_config(){auto c=defaultConfig();c.setupPassword="test-admin";c.usageToken="test-usage-token-123";return {c,0};}
int runtime_patch(const std::string&){++patches;return 204;}
void runtime_usage(const Usage&){++usage;}
uint64_t now_ms(){return clockMs;}
}
int main(){
    using namespace deskduo;
    portal_init();assert(routes.size()==6);
    for(auto credential:{"","Bearer test-usage-token-123","Basic wrong"}){
        auto e=exchange();e.headers["Authorization"]=credential;auto r=request(e);assert(dispatch(r)==ESP_FAIL);assert(e.status=="401 Unauthorized" && e.reads==0 && asyncCount==0);
    }
    for(auto origin:{"http://192.0.2.10.evil.test","http://192.0.2.10@evil.test"}){
        auto e=exchange();e.headers["Origin"]=origin;auto r=request(e);assert(dispatch(r)==ESP_FAIL && e.status=="403 Forbidden" && !e.reads);
    }
    {auto e=exchange();e.headers["Origin"]=std::string(300,'x');auto r=request(e);assert(dispatch(r)==ESP_FAIL && e.status=="403 Forbidden");}
    asyncFail=true;{auto e=exchange();auto r=request(e);assert(dispatch(r)==ESP_FAIL && !ota_busy());}asyncFail=false;
    taskFail=true;{auto e=exchange();auto r=request(e);assert(dispatch(r)==ESP_FAIL && !ota_busy() && asyncCount==completed);}taskFail=false;
    auto bad=exchange();bad.body.resize(1000);auto upload=request(bad);upload.content_len=4096;
    assert(dispatch(upload)==ESP_OK && ota_busy() && bad.reads==0);
    {auto e=exchange();auto r=request(e);assert(dispatch(r)==ESP_FAIL && e.status=="503 Service Unavailable" && !e.reads);}
    {auto e=exchange();auto r=request(e,"/api/config");assert(dispatch(r)==ESP_FAIL && e.status=="503 Service Unavailable" && patches==0 && !e.reads);}
    {auto e=exchange();auto r=request(e,"/api/firmware",HTTP_GET);assert(dispatch(r)==ESP_OK && e.response.find("\"busy\":true")!=std::string::npos && e.response.find("ota_0")!=std::string::npos);}
    {auto e=exchange();e.headers["Authorization"]="Bearer test-usage-token-123";e.headers["Content-Type"]="application/json";e.body=R"({"schemaVersion":1,"type":"usage","provider":"claude","updatedAt":"2026-10-04T03:00:00Z","primary":{"usedPercent":10,"windowMinutes":300,"resetsAt":"2026-10-04T08:00:00Z"}})";auto r=request(e,"/api/usage");dispatch(r);assert(e.reads>0 && e.status=="204 No Content" && usage==1);}
    runWorker();assert(!ota_busy() && completed==asyncCount && aborted==1 && selected==0 && bad.status=="408 Request Timeout");
    beginFail=true;{auto e=exchange();auto r=request(e);dispatch(r);runWorker();assert(!ota_busy() && completed==asyncCount && aborted==2 && selected==0 && e.status=="500 Internal Server Error");}beginFail=false;
    endFail=true;{auto e=exchange();auto r=request(e);dispatch(r);runWorker();assert(!ota_busy() && completed==asyncCount && selected==0 && e.status=="500 Internal Server Error");}endFail=false;
    {auto e=exchange();auto r=request(e);dispatch(r);try{runWorker();assert(false);}catch(const Reboot&){}assert(selected==1 && completed==asyncCount && e.status=="200 OK" && flash.size()==4096);maintenance.release();}
    ota_confirm_boot();assert(confirmed==1);bootState=ESP_OTA_IMG_VALID;ota_confirm_boot();assert(confirmed==1);
    std::cout<<"Production OTA routes, worker ownership, settings/usage admission and boot confirmation passed\n";
}
