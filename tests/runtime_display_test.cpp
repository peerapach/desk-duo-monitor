#include "../firmware/main/runtime.hpp"
#include "runtime_sdk.hpp"
#include <cstring>
#include <iostream>
#include <optional>
#include <string>
namespace {
std::string configBlob;
std::optional<uint8_t> savedMode,pendingMode;
int preferenceWrites=0,blobWrites=0,commits=0,reads=0;
bool commitFails=false,setFails=false,openFails=false;
deskduo::CodexUsageMode expectedDuringCommit=deskduo::CodexUsageMode::Both;
}
int xSemaphoreCreateMutex(){return 1;}
int xSemaphoreTake(int,int){return 1;}
int xSemaphoreGive(int){return 1;}
int nvs_flash_init(){return ESP_OK;}
int nvs_open(const char* ns,int access,nvs_handle_t* handle){assert(std::string(ns)=="deskduo" && access==NVS_READWRITE);*handle=1;return openFails?ESP_FAIL:ESP_OK;}
int nvs_get_blob(nvs_handle_t,const char* key,void* data,std::size_t* size){++reads;assert(std::string(key)=="config");if(!data){*size=configBlob.size();return ESP_OK;}assert(*size==configBlob.size());std::memcpy(data,configBlob.data(),*size);return ESP_OK;}
int nvs_set_blob(nvs_handle_t,const char* key,const void* data,std::size_t size){++blobWrites;assert(std::string(key)=="config");configBlob.assign(static_cast<const char*>(data),size);return ESP_OK;}
int nvs_get_u8(nvs_handle_t,const char* key,uint8_t* mode){++reads;assert(std::string(key)=="codex_layout");if(!savedMode)return ESP_ERR_NVS_NOT_FOUND;*mode=*savedMode;return ESP_OK;}
int nvs_set_u8(nvs_handle_t,const char* key,uint8_t mode){++preferenceWrites;assert(std::string(key)=="codex_layout" && mode<=1);if(setFails)return ESP_FAIL;pendingMode=mode;return ESP_OK;}
int nvs_commit(nvs_handle_t){++commits;assert(deskduo::runtime_display_mode()==expectedDuringCommit);if(commitFails)return ESP_FAIL;savedMode=pendingMode;return ESP_OK;}
void nvs_close(nvs_handle_t){pendingMode.reset();}
void bootloader_random_enable(){}
void bootloader_random_disable(){}
void esp_fill_random(void*,std::size_t){assert(false && "existing secrets must not be regenerated");}
int esp_read_mac(uint8_t*,int){return ESP_OK;}
int64_t esp_timer_get_time(){return 1000000;}
int main(){
 using namespace deskduo;
 auto cfg=defaultConfig();cfg.ssid="preserved-wifi";cfg.wifiPassword="secret-wifi";cfg.usageToken="0123456789abcdef";cfg.setupPassword="secret-admin";
 cfg.touch.minX=294;cfg.touch.maxX=3766;cfg.touch.minY=248;cfg.touch.maxY=3632;cfg.touch.invertX=true;
 configBlob=serializeConfig(cfg);const auto original=configBlob;
 runtime_init();assert(runtime_display_mode()==CodexUsageMode::Both && preferenceWrites==0 && blobWrites==0 && commits==0);
 const auto initialReads=reads;for(int i=0;i<20;++i){runtime_display_mode();runtime_snapshot();}assert(reads==initialReads);
 assert(runtime_display_patch(R"({"codexUsageMode":"bad"})")==400 && preferenceWrites==0);
 commitFails=true;assert(runtime_display_patch(R"({"codexUsageMode":"weekly"})")==503);
 assert(runtime_display_mode()==CodexUsageMode::Both && !savedMode);
 commitFails=false;setFails=true;const auto beforeCommits=commits;assert(runtime_display_patch(R"({"codexUsageMode":"weekly"})")==503 && commits==beforeCommits);setFails=false;
 openFails=true;assert(runtime_display_patch(R"({"codexUsageMode":"weekly"})")==503);openFails=false;
 assert(runtime_display_mode()==CodexUsageMode::Both);
 assert(runtime_display_patch(R"({"codexUsageMode":"weekly"})")==204 && savedMode==1 && runtime_display_mode()==CodexUsageMode::Weekly);
 runtime_init();assert(runtime_display_mode()==CodexUsageMode::Weekly);
 expectedDuringCommit=CodexUsageMode::Weekly;
 assert(runtime_display_patch(R"({"codexUsageMode":"both"})")==204 && savedMode==0 && runtime_display_mode()==CodexUsageMode::Both);
 assert(configBlob==original && blobWrites==0);
 // Diagnostic current config/generation and accepted data must come from runtime,
 // without NVS reads or acceptance of an obsolete worker result.
 const auto beforeDiagnosticReads=reads;
 auto view=runtime_weather_view();assert(view.generation==runtime_config().generation && !view.configured && !view.weather.data);
 Weather value;value.temperature=29.6;value.humidity=76;value.hourlyCount=4;
 runtime_weather({value,{}},view.generation);view=runtime_weather_view();assert(view.weather.data && view.weather.data->temperature==29.6);
 runtime_weather({std::nullopt,"json"},view.generation);assert(runtime_weather_view().weather.stale);
 runtime_weather({value,{}},view.generation+1);assert(runtime_weather_view().weather.stale);
 assert(reads==beforeDiagnosticReads && configBlob==original && blobWrites==0);
 const auto active=runtime_config().config;assert(completeSerialization(active,original));
 assert(parseStoredConfig(original).value);
 std::cout<<"Production runtime: missing-key default without write, scalar reload, open/set/commit failure, commit-before-publish, no tick NVS reads, config/secrets/calibration preserved\n";
}
