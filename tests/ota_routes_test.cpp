#include "../firmware/main/ota.hpp"
#include "../firmware/main/assets.hpp"
#include "../firmware/main/ota_policy.hpp"
#include "../firmware/main/portal.hpp"
#include "../firmware/main/input.hpp"
#include "../firmware/main/runtime.hpp"
#include <cstring>
#include <iostream>
#include <vector>
#include <ctime>
#include <cstdlib>
#include <new>
#include <limits>
#include "ota_image_fixture.hpp"
#include "esp_image_format.h"
namespace {
size_t allocationLimit=std::numeric_limits<size_t>::max();
size_t deniedAllocation=0;
struct AllocationLimit {
    explicit AllocationLimit(size_t limit) { allocationLimit=limit;deniedAllocation=0; }
    ~AllocationLimit() { allocationLimit=std::numeric_limits<size_t>::max(); }
};
}
void* operator new(size_t size) {
    if(size>=allocationLimit) {deniedAllocation=size;throw std::bad_alloc();}
    if(auto* memory=std::malloc(size?size:1))return memory;
    throw std::bad_alloc();
}
void* operator new[](size_t size) {return ::operator new(size);}
void operator delete(void* memory) noexcept {std::free(memory);}
void operator delete[](void* memory) noexcept {std::free(memory);}
void operator delete(void* memory,size_t) noexcept {std::free(memory);}
void operator delete[](void* memory,size_t) noexcept {std::free(memory);}
namespace {
time_t routeEpoch=0;
std::optional<uint8_t> appliedPercent;
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
extern "C" time_t time(time_t* out){if(out)*out=routeEpoch;return routeEpoch;}
size_t httpd_req_get_hdr_value_len(httpd_req_t* r,const char* name){return r->exchange->headers[name].size();}
int httpd_req_get_hdr_value_str(httpd_req_t* r,const char* name,char* out,size_t n){auto& s=r->exchange->headers[name];if(n<=s.size())return ESP_FAIL;std::memcpy(out,s.c_str(),s.size()+1);return ESP_OK;}
int httpd_req_recv(httpd_req_t* r,char* out,size_t n){auto& e=*r->exchange;++e.reads;n=std::min({n,size_t(257),e.body.size()-e.offset});std::memcpy(out,e.body.data()+e.offset,n);e.offset+=n;++clockMs;return n;}
int httpd_resp_set_status(httpd_req_t* r,const char* s){r->exchange->status=s;return ESP_OK;}
int httpd_resp_set_type(httpd_req_t* r,const char* type){r->exchange->contentType=type;return ESP_OK;}
int httpd_resp_set_hdr(httpd_req_t* r,const char* name,const char* value){r->exchange->responseHeaders[name]=value;return ESP_OK;}
int httpd_resp_send(httpd_req_t* r,const char* s,size_t n){if(r->exchange->sendResult!=ESP_OK)return r->exchange->sendResult;r->exchange->response.assign(s,n);return ESP_OK;}
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
std::optional<uint8_t> backlight_last_applied_percent(){return appliedPercent;}
RequestConfig runtime_config(){auto c=defaultConfig();c.setupPassword="test-admin";c.usageToken="test-usage-token-123";c.ssid="saved-network";c.wifiPassword="test-wifi-secret";c.weather.url="https://weather.test/data?key=test-query-secret";c.weather.headers={{"X-Key","test-header-secret"}};return {c,0};}
DisplayPreference displayPreference;
bool displaySave=true;int displayWrites=0;
DisplaySettings runtime_display_settings(){return displayPreference.settings();}
Language runtime_language(){return displayPreference.language();}
CodexUsageMode runtime_display_mode(){return displayPreference.get();}
int runtime_display_patch(const std::string& json){return displayPreference.patch(json,[](const DisplaySettings&){++displayWrites;return displaySave;});}
int runtime_patch(const std::string&){++patches;return 204;}
void runtime_usage(const Usage&){++usage;}
uint64_t now_ms(){return clockMs;}
std::string network_diagnostics_json(){return R"({"stationOnline":true,"weatherConfigured":true,"attempt":{"outcome":"dns_failure","httpStatus":null}})";}
std::string input_diagnostics_json(){return R"({"sequence":19,"lastPollTimeMs":20,"lastPressure":0,"pressureAccepted":false,"samples":[{"sequence":18,"timeMs":19,"rawX":123,"rawY":456,"pressure":200,"x":7,"y":8}]})";}
}
int main(int argc,char**){
    using namespace deskduo;
    portal_init();assert(routes.count({"/api/network",HTTP_GET}) && !routes.count({"/api/network",HTTP_POST}));assert(routes.size()==10);
    assert(serverConfig.max_uri_handlers==10 && serverConfig.max_open_sockets==4 && serverConfig.lru_purge_enable);
    // A full embedded page must remain available without page-sized application heap.
    const auto pageBytes=std::strlen(portal_html);
    {
        auto e=exchange();auto r=request(e,"/",HTTP_GET);
        e.response.reserve(pageBytes); // Fake transport storage is outside the measured dispatch.
        int result=ESP_FAIL;
        try {AllocationLimit limit(pageBytes);result=dispatch(r);}
        catch(const std::bad_alloc&) {
            std::cerr<<"Root route attempted allocation of "<<deniedAllocation
                     <<" bytes with page-sized allocation limit "<<pageBytes<<"\n";
            return 1;
        }
        assert(result==ESP_OK && e.status=="200 OK" && e.response==portal_html);
        assert(e.contentType=="text/html; charset=utf-8" && e.reads==0);
        assert(e.responseHeaders.at("Content-Security-Policy")=="default-src 'none'; script-src 'unsafe-inline'; style-src 'unsafe-inline'; connect-src 'self'; frame-ancestors 'none'; base-uri 'none'; form-action 'self'");
        assert(e.responseHeaders.at("X-Content-Type-Options")=="nosniff");
        assert(e.responseHeaders.at("Cache-Control")=="no-store");
        std::cout<<"Full embedded root body ("<<pageBytes<<" bytes) served under allocation limit\n";
    }
    for(auto credential:{"","Bearer test-usage-token-123","Basic wrong"}) {
        auto e=exchange();e.headers["Authorization"]=credential;auto r=request(e,"/",HTTP_GET);
        assert(dispatch(r)==ESP_FAIL && e.status=="401 Unauthorized" && e.response.empty());
        assert(e.responseHeaders.at("WWW-Authenticate")=="Basic realm=\"Desk Duo\"");
    }
    // A successful status must not swallow transport failure for HTML or temporary JSON.
    for(auto path:{"/","/api/network","/api/display"}) {
        auto e=exchange();e.sendResult=-77;auto r=request(e,path,HTTP_GET);
        assert(dispatch(r)==-77 && e.status=="200 OK");
    }
    for(auto method:{HTTP_GET,HTTP_POST}){
      for(auto credential:{"","Bearer test-usage-token-123","Basic wrong"}){
        auto e=exchange();e.headers["Authorization"]=credential;auto r=request(e,"/api/display",method);
        assert(dispatch(r)==ESP_FAIL && e.status=="401 Unauthorized" && e.reads==0);
      }
    }
    for(auto credential:{"","Bearer test-usage-token-123","Basic wrong"}) {
        auto e=exchange();e.headers["Authorization"]=credential;auto r=request(e,"/api/network",HTTP_GET);
        assert(dispatch(r)==ESP_FAIL && e.status=="401 Unauthorized" && e.reads==0);
    }
    {auto e=exchange();auto r=request(e,"/api/network",HTTP_GET);
     assert(dispatch(r)==ESP_OK && e.status=="200 OK" && e.reads==0);
     assert(e.response==network_diagnostics_json() && patches==0 && usage==0);}
    // Current settings remain administrator-only, with presence instead of secrets.
    for(auto credential:{"","Bearer test-usage-token-123","Basic wrong"}) {
        auto e=exchange();e.headers["Authorization"]=credential;auto r=request(e,"/api/config",HTTP_GET);
        assert(dispatch(r)==ESP_FAIL && e.status=="401 Unauthorized" && e.reads==0);
    }
    {auto e=exchange();auto r=request(e,"/api/config",HTTP_GET);
     assert(dispatch(r)==ESP_OK && e.status=="200 OK");
     assert(e.response.find(R"("currentSSID":"saved-network")")!=std::string::npos);
     assert(e.response.find(R"("wifiPasswordConfigured":true)")!=std::string::npos);
     assert(e.response.find(R"("usageTokenConfigured":true)")!=std::string::npos);
     assert(e.response.find(R"("headersConfigured":true)")!=std::string::npos);
     assert(e.response.find(R"("urlDetailsHidden":true)")!=std::string::npos);
     for(auto secret:{"test-admin","test-usage-token-123","test-wifi-secret","test-query-secret","test-header-secret"})assert(e.response.find(secret)==std::string::npos);
     assert(patches==0 && usage==0);}
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
        assert(dispatch(r)==ESP_OK && e.response==R"({"codexUsageMode":"both","language":"th","brightnessPercent":100,"brightnessAuto":false,"brightnessDayStart":"06:00","brightnessNightStart":"18:00","brightnessTargetPercent":100,"brightnessEffectivePercent":null,"brightnessPhase":"manual"})");
        e=exchange();e.headers["Content-Type"]="application/json";e.body=R"({"codexUsageMode":"weekly"})";r=request(e,"/api/display");
        assert(dispatch(r)==ESP_OK && e.status=="204 No Content" && e.response.empty());
        e=exchange();r=request(e,"/api/display",HTTP_GET);
        assert(dispatch(r)==ESP_OK && e.response==R"({"codexUsageMode":"weekly","language":"th","brightnessPercent":100,"brightnessAuto":false,"brightnessDayStart":"06:00","brightnessNightStart":"18:00","brightnessTargetPercent":100,"brightnessEffectivePercent":null,"brightnessPhase":"manual"})");
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

    // The authenticated display route must preserve sparse compatibility and
    // reject bad/origin/failed combined patches before publishing either field.
    {
        auto e=exchange();e.headers["Content-Type"]="application/json";e.body=R"({"language":"en"})";auto r=request(e,"/api/display");
        assert(dispatch(r)==ESP_OK && runtime_language()==Language::English && runtime_display_mode()==CodexUsageMode::Both);
        e=exchange();r=request(e,"/api/display",HTTP_GET);
        assert(dispatch(r)==ESP_OK && e.response==R"({"codexUsageMode":"both","language":"en","brightnessPercent":100,"brightnessAuto":false,"brightnessDayStart":"06:00","brightnessNightStart":"18:00","brightnessTargetPercent":100,"brightnessEffectivePercent":null,"brightnessPhase":"manual"})");
        for(auto bad:{R"({"language":"TH"})",R"({"language":true})",R"({"language":null})",R"({"language":"en","language":"th"})",R"({"language":"th","codexUsageMode":"invalid"})",R"({"language":"th","unknown":0})"}) {
            e=exchange();e.headers["Content-Type"]="application/json";e.body=bad;r=request(e,"/api/display");
            assert(dispatch(r)==ESP_FAIL && e.status=="400 Bad Request" && runtime_language()==Language::English && runtime_display_mode()==CodexUsageMode::Both);
        }
        displaySave=false;e=exchange();e.headers["Content-Type"]="application/json";e.body=R"({"language":"th","codexUsageMode":"weekly"})";r=request(e,"/api/display");
        assert(dispatch(r)==ESP_FAIL && e.status=="503 Service Unavailable" && runtime_language()==Language::English && runtime_display_mode()==CodexUsageMode::Both);displaySave=true;
        for(const auto& origin:std::vector<std::string>{"https://evil.test","http://192.0.2.10.evil.test","null",std::string(257,'x')}) {
            e=exchange();e.headers["Origin"]=origin;e.headers["Content-Type"]="application/json";e.body=R"({"language":"th"})";r=request(e,"/api/display");
            assert(dispatch(r)==ESP_FAIL && e.status=="403 Forbidden" && e.reads==0 && runtime_language()==Language::English);
        }
        e=exchange();e.headers["Origin"]="http://192.0.2.10";e.headers["Content-Type"]="application/json";e.body=R"({"language":"th","codexUsageMode":"weekly"})";r=request(e,"/api/display");
        assert(dispatch(r)==ESP_OK && runtime_language()==Language::Thai && runtime_display_mode()==CodexUsageMode::Weekly);
        e=exchange();r=request(e,"/api/display",HTTP_GET);
        assert(dispatch(r)==ESP_OK && e.response==R"({"codexUsageMode":"weekly","language":"th","brightnessPercent":100,"brightnessAuto":false,"brightnessDayStart":"06:00","brightnessNightStart":"18:00","brightnessTargetPercent":100,"brightnessEffectivePercent":null,"brightnessPhase":"manual"})");
        assert(patches==0 && usage==0);
    }

    {
      for(auto value:{5,50,100}) {
        auto e=exchange();e.headers["Content-Type"]="application/json";e.body="{\"brightnessPercent\":"+std::to_string(value)+"}";
        auto r=request(e,"/api/display");assert(dispatch(r)==ESP_OK && runtime_display_settings().brightnessPercent==value);
        assert(runtime_display_mode()==CodexUsageMode::Weekly && runtime_language()==Language::Thai);
        e=exchange();r=request(e,"/api/display",HTTP_GET);assert(dispatch(r)==ESP_OK);
        assert(e.response=="{\"codexUsageMode\":\"weekly\",\"language\":\"th\",\"brightnessPercent\":"+std::to_string(value)+",\"brightnessAuto\":false,\"brightnessDayStart\":\"06:00\",\"brightnessNightStart\":\"18:00\",\"brightnessTargetPercent\":"+std::to_string(value)+",\"brightnessEffectivePercent\":null,\"brightnessPhase\":\"manual\"}");
      }
      for(auto bad:{R"({"brightnessPercent":4})",R"({"brightnessPercent":101})",R"({"brightnessPercent":50.5})",R"({"brightnessPercent":true})",R"({"brightnessPercent":null})",R"({"brightnessPercent":5,"brightnessPercent":100})",R"({"brightnessPercent":5,"unknown":1})"}) {
        auto e=exchange();e.headers["Content-Type"]="application/json";e.body=bad;auto r=request(e,"/api/display");
        const auto beforeWrites=displayWrites;assert(dispatch(r)==ESP_FAIL && e.status=="400 Bad Request" && runtime_display_settings().brightnessPercent==100 && displayWrites==beforeWrites);
      }
    }

    {
      auto post=[&](const char* body,int expected){auto e=exchange();e.headers["Content-Type"]="application/json";e.body=body;auto r=request(e,"/api/display");auto writes=displayWrites;dispatch(r);assert(e.status==(expected==204?"204 No Content":"400 Bad Request"));assert(displayWrites==writes+(expected==204?1:0));};
      post(R"({"brightnessAuto":true,"brightnessPercent":11,"brightnessDayStart":"07:15","brightnessNightStart":"19:45"})",204);
      auto get=[&](const char* phase,int target,const char* effective){auto e=exchange();auto r=request(e,"/api/display",HTTP_GET);auto writes=displayWrites;assert(dispatch(r)==ESP_OK && e.reads==0 && writes==displayWrites);assert(e.response.find(R"("brightnessPercent":11,"brightnessAuto":true,"brightnessDayStart":"07:15","brightnessNightStart":"19:45")")!=std::string::npos);assert(e.response.find(std::string("\"brightnessTargetPercent\":")+std::to_string(target))!=std::string::npos);assert(e.response.find(std::string("\"brightnessEffectivePercent\":")+effective)!=std::string::npos);assert(e.response.find(std::string("\"brightnessPhase\":\"")+phase+"\"")!=std::string::npos);};
      get("clock_wait",11,"null");
      setenv("TZ","ICT-7",1);tzset();std::tm local{};local.tm_year=126;local.tm_mon=9;local.tm_mday=16;local.tm_hour=20;routeEpoch=std::mktime(&local);
      appliedPercent=11;get("night",6,"11");appliedPercent=6;get("night",6,"6");
      local.tm_hour=8;routeEpoch=std::mktime(&local);get("day",11,"6");
      for(auto bad:{R"({"brightnessAuto":"true"})",R"({"brightnessAuto":true,"brightnessAuto":false})",R"({"brightnessDayStart":"7:15"})",R"({"brightnessNightStart":"24:00"})",R"({"brightnessDayStart":"19:45"})",R"({"brightnessNightStart":"07:15"})",R"({"brightnessPhase":"night"})",R"({"brightnessTargetPercent":50})",R"({"brightnessEffectivePercent":null})"})post(bad,400);
      post(R"({"brightnessAuto":false,"brightnessDayStart":"19:45","brightnessNightStart":"19:45"})",400);
      post(R"({"brightnessAuto":false})",204);
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
    {auto e=exchange();auto r=request(e);const auto beforeWrites=displayWrites;assert(dispatch(r)==ESP_FAIL && e.status=="503 Service Unavailable" && !e.reads && displayWrites==beforeWrites);}
    {auto e=exchange();auto r=request(e,"/api/config");assert(dispatch(r)==ESP_FAIL && e.status=="503 Service Unavailable" && e.response=="Firmware maintenance busy; retry shortly" && patches==0 && !e.reads);}
    {auto e=exchange();auto r=request(e,"/api/display");assert(dispatch(r)==ESP_FAIL && e.status=="503 Service Unavailable" && !e.reads);}
    {auto e=exchange();auto r=request(e,"/api/network",HTTP_GET);assert(dispatch(r)==ESP_OK && e.response==network_diagnostics_json() && !e.reads);}
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
