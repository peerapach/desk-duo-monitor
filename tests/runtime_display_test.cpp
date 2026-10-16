#include "../firmware/main/runtime.hpp"
#include "runtime_sdk.hpp"
#include <cstring>
#include <iostream>
#include <optional>
#include <string>
namespace {
std::string configBlob;
std::optional<uint8_t> savedPreference;
std::optional<uint16_t> savedV2;
int v2Reads=0,v2ReadError=ESP_OK;
uint8_t legacyMode=0;
int preferenceWrites=0,blobWrites=0,commits=0,reads=0,legacyReads=0;
int preferenceReadError=ESP_OK,legacyReadError=ESP_OK;
bool commitFails=false,setFails=false,openFails=false;
deskduo::DisplaySettings expectedDuringCommit;
}
int xSemaphoreCreateMutex(){return 1;}
int xSemaphoreTake(int,int){return 1;}
int xSemaphoreGive(int){return 1;}
int nvs_flash_init(){return ESP_OK;}
int nvs_open(const char* ns,int access,nvs_handle_t* handle){assert(std::string(ns)=="deskduo" && access==NVS_READWRITE);*handle=1;return openFails?ESP_FAIL:ESP_OK;}
int nvs_get_blob(nvs_handle_t,const char* key,void* data,std::size_t* size){++reads;assert(std::string(key)=="config");if(!data){*size=configBlob.size();return ESP_OK;}assert(*size==configBlob.size());std::memcpy(data,configBlob.data(),*size);return ESP_OK;}
int nvs_set_blob(nvs_handle_t,const char* key,const void* data,std::size_t size){++blobWrites;assert(std::string(key)=="config");configBlob.assign(static_cast<const char*>(data),size);return ESP_OK;}
int nvs_get_u8(nvs_handle_t,const char* key,uint8_t* value){
 ++reads;
 if(std::string(key)=="codex_layout"){++legacyReads;if(legacyReadError!=ESP_OK)return legacyReadError;*value=legacyMode;return ESP_OK;}
 assert(std::string(key)=="ui_pref");
 if(preferenceReadError!=ESP_OK)return preferenceReadError;
 if(!savedPreference)return ESP_ERR_NVS_NOT_FOUND;
 *value=*savedPreference;return ESP_OK;
}
int nvs_set_u8(nvs_handle_t,const char*,uint8_t){assert(false && "old keys immutable");return ESP_FAIL;}
int nvs_get_u16(nvs_handle_t,const char* key,uint16_t* value){
 ++reads;++v2Reads;assert(std::string(key)=="ui_pref_v2");
 if(v2ReadError!=ESP_OK)return v2ReadError;
 if(!savedV2)return ESP_ERR_NVS_NOT_FOUND;
 *value=*savedV2;return ESP_OK;
}
int nvs_set_u16(nvs_handle_t,const char* key,uint16_t value){
 ++preferenceWrites;assert(std::string(key)=="ui_pref_v2");
 if(setFails)return ESP_FAIL;
 savedV2=value;return ESP_OK;
}
int nvs_commit(nvs_handle_t){
 ++commits;const auto active=deskduo::runtime_display_settings();
 assert(active.mode==expectedDuringCommit.mode && active.language==expectedDuringCommit.language && active.brightnessPercent==expectedDuringCommit.brightnessPercent);
 return commitFails?ESP_FAIL:ESP_OK;
}
void nvs_close(nvs_handle_t){}
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
 auto check=[&](CodexUsageMode mode,Language language,int brightness){
   const auto active=runtime_display_settings();assert(active.mode==mode && active.language==language && active.brightnessPercent==brightness);
 };
 auto load=[&](CodexUsageMode mode,Language language,int brightness){
   const auto writes=preferenceWrites,commitCount=commits;
   runtime_init();check(mode,language,brightness);
   assert(preferenceWrites==writes && commits==commitCount && configBlob==original && blobWrites==0);
 };
 load(CodexUsageMode::Both,Language::Thai,100);assert(v2Reads==1);
 const auto initialReads=reads;for(int i=0;i<20;++i){runtime_display_settings();runtime_language();runtime_snapshot();}assert(reads==initialReads);
 for(int old=0;old<=3;++old){savedPreference=old;load(static_cast<CodexUsageMode>(old&1),static_cast<Language>(old>>1),100);}
 savedPreference.reset();legacyMode=1;load(CodexUsageMode::Weekly,Language::Thai,100);
 savedPreference=255;load(CodexUsageMode::Weekly,Language::Thai,100);
 preferenceReadError=ESP_ERR_NVS_TYPE_MISMATCH;load(CodexUsageMode::Weekly,Language::Thai,100);
 preferenceReadError=ESP_OK;legacyMode=255;load(CodexUsageMode::Both,Language::Thai,100);
 legacyReadError=ESP_ERR_NVS_TYPE_MISMATCH;load(CodexUsageMode::Both,Language::Thai,100);legacyReadError=ESP_OK;
 for(uint16_t invalid:{uint16_t(0),uint16_t(19),uint16_t(404),uint16_t(65535)}){
   savedV2=invalid;savedPreference=2;load(CodexUsageMode::Both,Language::English,100);
 }
 v2ReadError=ESP_ERR_NVS_TYPE_MISMATCH;load(CodexUsageMode::Both,Language::English,100);v2ReadError=ESP_OK;
 // Valid v2 must bypass even operational failures in obsolete keys.
 savedV2=203;preferenceReadError=ESP_FAIL;legacyReadError=ESP_FAIL;
 auto oldReads=legacyReads;load(CodexUsageMode::Weekly,Language::English,50);assert(oldReads==legacyReads);
 for(int percent:{5,50,100})for(int bits=0;bits<4;++bits){
   savedV2=(percent<<2)|bits;
   load(static_cast<CodexUsageMode>(bits&1),static_cast<Language>(bits>>1),percent);
 }
 savedV2=203;load(CodexUsageMode::Weekly,Language::English,50);
 preferenceReadError=ESP_OK;legacyReadError=ESP_OK;
 for(auto bad:{"{}",R"({"brightnessPercent":4})",R"({"brightnessPercent":101})",R"({"brightnessPercent":50.5})",R"({"brightnessPercent":true})",R"({"brightnessPercent":null})",R"({"brightnessPercent":"50"})",R"({"brightnessPercent":5,"brightnessPercent":100})",R"({"brightnessPercent":5,"x":1})",R"({"language":"de"})"}){
   const auto writes=preferenceWrites;assert(runtime_display_patch(bad)==400 && preferenceWrites==writes);check(CodexUsageMode::Weekly,Language::English,50);
 }
 auto patch=[&](const char* json,uint16_t encoded,CodexUsageMode mode,Language language,int percent){
   assert(savedPreference==2 && legacyMode==255);
 expectedDuringCommit=runtime_display_settings();auto writes=preferenceWrites,count=commits;
   assert(runtime_display_patch(json)==204 && savedV2==encoded && preferenceWrites==writes+1 && commits==count+1);
   check(mode,language,percent);load(mode,language,percent);
 };
 patch(R"({"brightnessPercent":5})",23,CodexUsageMode::Weekly,Language::English,5);
 patch(R"({"language":"th"})",21,CodexUsageMode::Weekly,Language::Thai,5);
 patch(R"({"codexUsageMode":"both"})",20,CodexUsageMode::Both,Language::Thai,5);
 patch(R"({"brightnessPercent":100,"language":"en","codexUsageMode":"weekly"})",403,CodexUsageMode::Weekly,Language::English,100);
 expectedDuringCommit=runtime_display_settings();auto writes=preferenceWrites,count=commits;
 openFails=true;assert(runtime_display_patch(R"({"brightnessPercent":50})")==503);openFails=false;
 assert(preferenceWrites==writes && commits==count && savedV2==403);check(CodexUsageMode::Weekly,Language::English,100);
 setFails=true;assert(runtime_display_patch(R"({"brightnessPercent":50})")==503);setFails=false;
 assert(preferenceWrites==writes+1 && commits==count && savedV2==403);check(CodexUsageMode::Weekly,Language::English,100);
 commitFails=true;assert(runtime_display_patch(R"({"brightnessPercent":50,"language":"th","codexUsageMode":"both"})")==503);
 assert(savedV2==200);check(CodexUsageMode::Weekly,Language::English,100);
 commitFails=false;load(CodexUsageMode::Both,Language::Thai,50);
 // An artificial failed commit keeps active state old; immediate setter means
 // reboot sees the complete new triple, never a partial one.
 for(int boundary=0;boundary<3;++boundary){
   savedV2.reset();savedPreference.reset();v2ReadError=boundary==0?ESP_FAIL:ESP_OK;
   preferenceReadError=boundary==1?ESP_FAIL:ESP_OK;legacyReadError=boundary==2?ESP_FAIL:ESP_OK;
   writes=preferenceWrites;count=commits;bool failed=false;
   try{runtime_init();}catch(int error){failed=error==ESP_FAIL;}
   assert(failed && preferenceWrites==writes && commits==count);
 }
 v2ReadError=preferenceReadError=legacyReadError=ESP_OK;legacyMode=0;load(CodexUsageMode::Both,Language::Thai,100);
 assert(savedPreference==std::nullopt && legacyMode==0 && completeSerialization(runtime_config().config,original));
 // Diagnostic data keeps the existing source-generation admission behavior.
 const auto beforeDiagnosticReads=reads;
 auto view=runtime_weather_view();assert(view.generation==runtime_config().generation && !view.configured && !view.weather.data);
 Weather value;value.temperature=29.6;value.humidity=76;value.hourlyCount=4;
 runtime_weather({value,{}},view.generation);view=runtime_weather_view();assert(view.weather.data && view.weather.data->temperature==29.6);
 runtime_weather({std::nullopt,"json"},view.generation);assert(runtime_weather_view().weather.stale);
 runtime_weather({value,{}},view.generation+1);assert(runtime_weather_view().weather.stale);
 assert(reads==beforeDiagnosticReads && configBlob==original && blobWrites==0);
 assert(completeSerialization(runtime_config().config,original) && parseStoredConfig(original).value);
 std::cout<<"Production runtime: v2 u16 immediate single-write, exact 5/50/100 encoding, authoritative key skips obsolete reads, invalid/type fallback no writes, read errors remain fatal, reload, open/set/commit failure, coherent commit-before-publish, no tick NVS reads, config/secrets/calibration preserved\n";
}
