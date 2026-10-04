#include "../firmware/main/ota.hpp"
#include "../firmware/main/ota_policy.hpp"
#include "../firmware/main/portal.hpp"
#include "../firmware/main/input.hpp"
#include "../firmware/main/runtime.hpp"
#include <cstring>
#include <iostream>
#include <vector>
#include "ota_image_fixture.hpp"
#include "esp_image_format.h"
namespace {
std::map<std::pair<std::string,int>,Handler> routes;
TaskFunction task=nullptr;void* taskArg=nullptr;
int asyncCount=0,completed=0,selected=0,aborted=0,confirmed=0,patches=0,usage=0;
bool asyncFail=false,taskFail=false,endFail=false,beginFail=false;
uint64_t clockMs=0;
esp_ota_img_states_t bootState=ESP_OTA_IMG_PENDING_VERIFY;
esp_partition_t target{0x1e0000,"ota_1",0x210000},running{0x1e0000,"ota_0",0x30000};
struct TaskExit{};struct Reboot{};
std::vector<unsigned char> flash=ota_image_fixture();
size_t flashWritten=0;
httpd_config_t serverConfig{};
std::vector<Exchange*> sessions;
bool acceptConnection(Exchange& exchange) {
    if(sessions.size()==size_t(serverConfig.max_open_sockets)) {
        if(!serverConfig.lru_purge_enable)return false;
        auto idle=std::find_if(sessions.begin(),sessions.end(),[](auto* session){return !session->for_async_req;});
        if(idle==sessions.end())return false;
        sessions.erase(idle);
    }
    sessions.push_back(&exchange);return true;
}

Exchange exchange() { Exchange e;e.headers={{"Authorization",deskduo::basicCredential("test-admin")},{"Content-Type","application/octet-stream"},{"X-DeskDuo-Update","1"},{"Host","192.0.2.10"}};auto image=ota_image_fixture();e.body.assign(image.begin(),image.end());return e; }
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
int httpd_req_async_handler_begin(httpd_req_t* r,httpd_req_t** out){if(asyncFail)return ESP_FAIL;*out=new httpd_req_t(*r);r->exchange->for_async_req=true;++asyncCount;return ESP_OK;}
int httpd_req_async_handler_complete(httpd_req_t* r){r->exchange->for_async_req=false;delete r;++completed;return ESP_OK;}
int httpd_req_to_sockfd(httpd_req_t*){return -1;}
int httpd_sess_trigger_close(httpd_handle_t,int){return ESP_OK;}
int httpd_start(httpd_handle_t* out,const httpd_config_t* c){serverConfig=*c;assert(c->max_open_sockets>=4);*out=nullptr;return ESP_OK;}
int httpd_register_uri_handler(httpd_handle_t,const httpd_uri_t* r){routes[{r->uri,r->method}]=r->handler;return ESP_OK;}
const esp_partition_t* esp_ota_get_next_update_partition(const esp_partition_t*){return &target;}
const esp_partition_t* esp_ota_get_running_partition(){return &running;}
int esp_ota_get_state_partition(const esp_partition_t*,esp_ota_img_states_t* state){*state=bootState;return ESP_OK;}
int esp_ota_begin(const esp_partition_t* p,size_t n,esp_ota_handle_t* handle){assert(p==&target && n<=target.size);*handle=123;flashWritten=0;flash.resize(std::max(flash.size(),(n+4095)&~size_t(4095)),0xff);std::fill_n(flash.begin(),(n+4095)&~size_t(4095),0xff);return beginFail?ESP_FAIL:ESP_OK;}
int esp_ota_write(esp_ota_handle_t h,const void* data,size_t n){assert(h==123);auto* p=static_cast<const unsigned char*>(data);std::copy_n(p,n,flash.begin()+flashWritten);flashWritten+=n;return ESP_OK;}
int esp_ota_end(esp_ota_handle_t h){assert(h==123);return endFail || !ota_fixture_verified_size(flash)?ESP_FAIL:ESP_OK;}
esp_err_t esp_image_verify(esp_image_load_mode_t mode,const esp_partition_pos_t* pos,esp_image_metadata_t* data){assert(mode==ESP_IMAGE_VERIFY && pos->offset==target.address && pos->size==target.size);data->image_len=ota_fixture_verified_size(flash);return data->image_len?ESP_OK:ESP_FAIL;}
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
DisplayPreference displayPreference;
bool displaySave=true;
CodexUsageMode runtime_display_mode(){return displayPreference.get();}
int runtime_display_patch(const std::string& json){return displayPreference.patch(json,[](CodexUsageMode){return displaySave;});}
int runtime_patch(const std::string&){++patches;return 204;}
void runtime_usage(const Usage&){++usage;}
uint64_t now_ms(){return clockMs;}
std::string input_diagnostics_json(){return R"({"sequence":19,"lastPollTimeMs":20,"lastPressure":0,"pressureAccepted":false,"samples":[{"sequence":18,"timeMs":19,"rawX":123,"rawY":456,"pressure":200,"x":7,"y":8}]})";}
}
int main(int argc,char**){
    using namespace deskduo;
    portal_init();assert(routes.size()==9);
    assert(serverConfig.max_uri_handlers>=9);
    for(auto method:{HTTP_GET,HTTP_POST}){
      for(auto credential:{"","Bearer test-usage-token-123","Basic wrong"}){
        auto e=exchange();e.headers["Authorization"]=credential;auto r=request(e,"/api/display",method);
        assert(dispatch(r)==ESP_FAIL && e.status=="401 Unauthorized" && e.reads==0);
      }
    }
    assert(routes.count({"/api/touch",HTTP_GET}) && !routes.count({"/api/touch",HTTP_POST}));
    for(auto credential:{"","Bearer test-usage-token-123","Basic wrong"}) {
        auto e=exchange();e.headers["Authorization"]=credential;auto r=request(e,"/api/touch",HTTP_GET);
        assert(dispatch(r)==ESP_FAIL && e.status=="401 Unauthorized" && e.reads==0);
    }
    {auto e=exchange();auto r=request(e,"/api/touch",HTTP_GET);
     assert(dispatch(r)==ESP_OK && e.status=="200 OK" && e.reads==0);
     assert(e.response==R"({"sequence":19,"lastPollTimeMs":20,"lastPressure":0,"pressureAccepted":false,"samples":[{"sequence":18,"timeMs":19,"rawX":123,"rawY":456,"pressure":200,"x":7,"y":8}]})");
     assert(patches==0 && usage==0);}

    {
        auto e=exchange();auto r=request(e,"/api/display",HTTP_GET);
        assert(dispatch(r)==ESP_OK && e.response==R"({"codexUsageMode":"both"})");
        e=exchange();e.headers["Content-Type"]="application/json";e.body=R"({"codexUsageMode":"weekly"})";r=request(e,"/api/display");
        assert(dispatch(r)==ESP_OK && e.status=="204 No Content");
        e=exchange();r=request(e,"/api/display",HTTP_GET);
        assert(dispatch(r)==ESP_OK && e.response==R"({"codexUsageMode":"weekly"})");
        displaySave=false;e=exchange();e.headers["Content-Type"]="application/json";e.body=R"({"codexUsageMode":"both"})";r=request(e,"/api/display");
        assert(dispatch(r)==ESP_FAIL && e.status=="503 Service Unavailable");
        assert(runtime_display_mode()==CodexUsageMode::Weekly);displaySave=true;
        for(auto bad:{"{}",R"({"codexUsageMode":"invalid"})",R"({"codexUsageMode":"both","other":0})",R"({"codexUsageMode":"both","codexUsageMode":"weekly"})",R"({"codexUsageMode":"both"} garbage)"}){
            e=exchange();e.headers["Content-Type"]="application/json";e.body=bad;r=request(e,"/api/display");
            assert(dispatch(r)==ESP_FAIL && e.status=="400 Bad Request");assert(runtime_display_mode()==CodexUsageMode::Weekly);
        }
        e=exchange();e.headers["Content-Type"]="application/json";e.body=std::string(513,' ');r=request(e,"/api/display");
        assert(dispatch(r)==ESP_FAIL && e.status=="413 Content Too Large" && e.reads==0);
        e=exchange();e.headers["Content-Type"]="application/json";e.body=R"({"codexUsageMode":"both"})";e.body.resize(512,' ');r=request(e,"/api/display");
        assert(dispatch(r)==ESP_OK && e.status=="204 No Content");
        assert(patches==0 && usage==0);
    }

    if(argc>1) {
        auto active=exchange();Exchange idle1,idle2,idle3;auto status=exchange(),usageClient=exchange();
        assert(acceptConnection(active));auto r=request(active);assert(dispatch(r)==ESP_OK);
        assert(active.for_async_req && acceptConnection(idle1) && acceptConnection(idle2) && acceptConnection(idle3));
        assert(acceptConnection(status) && acceptConnection(usageClient));
        assert(active.for_async_req && std::find(sessions.begin(),sessions.end(),&active)!=sessions.end());
        assert(sessions.size()==4);
        auto statusRequest=request(status,"/api/firmware",HTTP_GET);
        assert(dispatch(statusRequest)==ESP_OK && status.status=="200 OK");
        usageClient.headers["Authorization"]="Bearer test-usage-token-123";
        usageClient.headers["Content-Type"]="application/json";
        usageClient.body=R"({"schemaVersion":1,"type":"usage","provider":"codex","updatedAt":"2026-10-04T03:00:00Z"})";
        auto usageRequest=request(usageClient,"/api/usage");
        assert(dispatch(usageRequest)==ESP_OK && usageClient.status=="204 No Content" && usage==1);
        endFail=true;runWorker();assert(!ota_busy() && !active.for_async_req && completed==asyncCount);
        std::cout<<"Idle connections reclaimed while async upload remained protected\n";return 0;
    }

    for(auto credential:{"","Bearer test-usage-token-123","Basic wrong"}){
        auto e=exchange();e.headers["Authorization"]=credential;auto r=request(e);assert(dispatch(r)==ESP_FAIL);assert(e.status=="401 Unauthorized" && e.reads==0 && asyncCount==0);
    }
    for(auto origin:{"http://192.0.2.10.evil.test","http://192.0.2.10@evil.test"}){
        auto e=exchange();e.headers["Origin"]=origin;auto r=request(e);assert(dispatch(r)==ESP_FAIL && e.status=="403 Forbidden" && !e.reads);
    }
    {auto e=exchange();e.headers["Origin"]=std::string(300,'x');auto r=request(e);assert(dispatch(r)==ESP_FAIL && e.status=="403 Forbidden");}
    asyncFail=true;{auto e=exchange();auto r=request(e);assert(dispatch(r)==ESP_FAIL && !ota_busy());}asyncFail=false;
    taskFail=true;{auto e=exchange();auto r=request(e);assert(dispatch(r)==ESP_FAIL && !ota_busy() && asyncCount==completed);}taskFail=false;
    auto bad=exchange();bad.body.resize(1000);auto upload=request(bad);upload.content_len=8192;
    assert(dispatch(upload)==ESP_OK && ota_busy() && bad.reads==0);
    {auto e=exchange();auto r=request(e);assert(dispatch(r)==ESP_FAIL && e.status=="503 Service Unavailable" && !e.reads);}
    {auto e=exchange();auto r=request(e,"/api/config");assert(dispatch(r)==ESP_FAIL && e.status=="503 Service Unavailable" && patches==0 && !e.reads);}
    {auto e=exchange();auto r=request(e,"/api/display");assert(dispatch(r)==ESP_FAIL && e.status=="503 Service Unavailable" && !e.reads);}
    {auto e=exchange();auto r=request(e,"/api/firmware",HTTP_GET);assert(dispatch(r)==ESP_OK && e.response.find("\"busy\":true")!=std::string::npos && e.response.find("ota_0")!=std::string::npos);}
    {auto e=exchange();e.headers["Authorization"]="Bearer test-usage-token-123";e.headers["Content-Type"]="application/json";e.body=R"({"schemaVersion":1,"type":"usage","provider":"claude","updatedAt":"2026-10-04T03:00:00Z","primary":{"usedPercent":10,"windowMinutes":300,"resetsAt":"2026-10-04T08:00:00Z"}})";auto r=request(e,"/api/usage");dispatch(r);assert(e.reads>0 && e.status=="204 No Content" && usage==1);}
    runWorker();assert(!ota_busy() && completed==asyncCount && aborted==1 && selected==0 && bad.status=="408 Request Timeout");
    beginFail=true;{auto e=exchange();auto r=request(e);dispatch(r);runWorker();assert(!ota_busy() && completed==asyncCount && aborted==2 && selected==0 && e.status=="500 Internal Server Error");}beginFail=false;
    endFail=true;{auto e=exchange();auto r=request(e);dispatch(r);runWorker();assert(!ota_busy() && completed==asyncCount && selected==0 && e.status=="500 Internal Server Error");}endFail=false;
    {
        flash=ota_image_fixture();const auto original=flash;
        auto e=exchange();e.body.resize(4096);auto r=request(e);
        assert(r.content_len==4096);dispatch(r);
        bool rebooted=false;try{runWorker();}catch(const Reboot&){rebooted=true;}
        assert(!rebooted && selected==0 && e.status=="400 Bad Request");
        assert(flash==original && flashWritten==4096 && !ota_busy() && completed==asyncCount);
    }
    {auto e=exchange();auto r=request(e);dispatch(r);try{runWorker();assert(false);}catch(const Reboot&){}assert(selected==1 && completed==asyncCount && e.status=="200 OK" && flashWritten==8192);maintenance.release();}
    ota_confirm_boot();assert(confirmed==1);bootState=ESP_OTA_IMG_VALID;ota_confirm_boot();assert(confirmed==1);
    std::cout<<"Production OTA routes, worker ownership, settings/usage admission and boot confirmation passed\n";
}
