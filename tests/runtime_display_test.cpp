#include "../firmware/main/runtime.hpp"
#include "runtime_sdk.hpp"
#include <cstring>
#include <iostream>
#include <optional>
#include <string>
namespace {
std::string configBlob;
std::optional<uint8_t> savedPreference;
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
int nvs_set_u8(nvs_handle_t,const char* key,uint8_t value){
 ++preferenceWrites;assert(std::string(key)=="ui_pref" && value<=3);
 if(setFails)return ESP_FAIL;
 // Pinned IDF writes on set; commit is not a transaction that rolls it back.
 savedPreference=value;return ESP_OK;
}
int nvs_commit(nvs_handle_t){
 ++commits;const auto active=deskduo::runtime_display_settings();
 assert(active.mode==expectedDuringCommit.mode && active.language==expectedDuringCommit.language);
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
 runtime_init();assert(runtime_display_mode()==CodexUsageMode::Both && runtime_language()==Language::Thai && preferenceWrites==0 && blobWrites==0 && commits==0);
 const auto initialReads=reads;for(int i=0;i<20;++i){runtime_display_settings();runtime_language();runtime_snapshot();}assert(reads==initialReads);
 for(auto bad:{"{}",R"({"language":"de"})",R"({"language":0})",R"({"language":null})",R"({"language":"en","language":"th"})",R"({"language":"th","x":1})",R"({"language":"en","codexUsageMode":false})",R"({"codexUsageMode":"bad"})"})assert(runtime_display_patch(bad)==400);
 assert(preferenceWrites==0);
 assert(runtime_display_patch(R"({"language":"en"})")==204 && savedPreference==2);
 assert(runtime_display_mode()==CodexUsageMode::Both && runtime_language()==Language::English && legacyMode==0);
 runtime_init();assert(runtime_language()==Language::English);
 expectedDuringCommit={CodexUsageMode::Both,Language::English};
 assert(runtime_display_patch(R"({"codexUsageMode":"weekly"})")==204 && savedPreference==3);
 assert(runtime_display_mode()==CodexUsageMode::Weekly && runtime_language()==Language::English && legacyMode==0);
 runtime_init();assert(runtime_display_mode()==CodexUsageMode::Weekly && runtime_language()==Language::English);
 expectedDuringCommit={CodexUsageMode::Weekly,Language::English};
 auto beforeCommits=commits,beforeWrites=preferenceWrites;
 setFails=true;assert(runtime_display_patch(R"({"codexUsageMode":"both","language":"th"})")==503);
 setFails=false;assert(commits==beforeCommits && preferenceWrites==beforeWrites+1 && savedPreference==3);
 runtime_init();assert(runtime_display_mode()==CodexUsageMode::Weekly && runtime_language()==Language::English);
 openFails=true;assert(runtime_display_patch(R"({"language":"th"})")==503);openFails=false;
 assert(runtime_language()==Language::English && savedPreference==3);
 // An artificial commit failure must not publish active state. The pinned SDK
 // may already have durably set the entire scalar, so reboot sees the whole
 // new pair; it can never see a mixed language/mode combination.
 commitFails=true;beforeWrites=preferenceWrites;
 assert(runtime_display_patch(R"({"codexUsageMode":"both","language":"th"})")==503);
 assert(runtime_display_mode()==CodexUsageMode::Weekly && runtime_language()==Language::English && savedPreference==0 && preferenceWrites==beforeWrites+1);
 commitFails=false;runtime_init();assert(runtime_display_mode()==CodexUsageMode::Both && runtime_language()==Language::Thai);
 expectedDuringCommit={CodexUsageMode::Both,Language::Thai};beforeWrites=preferenceWrites;
 assert(runtime_display_patch(R"({"codexUsageMode":"weekly","language":"en"})")==204 && savedPreference==3 && preferenceWrites==beforeWrites+1);
 assert(runtime_display_mode()==CodexUsageMode::Weekly && runtime_language()==Language::English);
 assert(configBlob==original && blobWrites==0 && legacyMode==0);
 savedPreference.reset();legacyMode=1;beforeWrites=preferenceWrites;beforeCommits=commits;
 runtime_init();assert(runtime_display_mode()==CodexUsageMode::Weekly && runtime_language()==Language::Thai && beforeWrites==preferenceWrites && beforeCommits==commits);
 savedPreference=255;runtime_init();assert(runtime_display_mode()==CodexUsageMode::Weekly && runtime_language()==Language::Thai && savedPreference==255 && beforeWrites==preferenceWrites);
 // A stored value with a different NVS type is invalid preference data,
 // not a fatal storage operation; the legacy layout is only a fallback.
 preferenceReadError=ESP_ERR_NVS_TYPE_MISMATCH;beforeWrites=preferenceWrites;beforeCommits=commits;
 try{runtime_init();}catch(int error){std::cerr<<"Stored preference type unexpectedly fatal: "<<error<<"\n";assert(false);}
 assert(runtime_language()==Language::Thai && runtime_display_mode()==CodexUsageMode::Weekly);
 assert(preferenceWrites==beforeWrites && commits==beforeCommits && configBlob==original && blobWrites==0);
 preferenceReadError=ESP_OK;savedPreference=2;legacyMode=255;const auto oldLegacyReads=legacyReads;
 runtime_init();assert(runtime_display_mode()==CodexUsageMode::Both && runtime_language()==Language::English && legacyReads==oldLegacyReads);
 assert(preferenceWrites==beforeWrites && commits==beforeCommits && configBlob==original && blobWrites==0);
 // Genuine read failures must reach the existing SDK fatal-check boundary.
 preferenceReadError=ESP_FAIL;bool storageFailure=false;
 try{runtime_init();}catch(int error){storageFailure=error==ESP_FAIL;}
 assert(storageFailure && legacyReads==oldLegacyReads && preferenceWrites==beforeWrites && commits==beforeCommits);
 preferenceReadError=ESP_OK;savedPreference.reset();legacyMode=1;legacyReadError=ESP_FAIL;storageFailure=false;
 try{runtime_init();}catch(int error){storageFailure=error==ESP_FAIL;}
 assert(storageFailure && preferenceWrites==beforeWrites && commits==beforeCommits && configBlob==original && blobWrites==0);
 legacyReadError=ESP_OK;runtime_init();assert(completeSerialization(runtime_config().config,original));
 // Diagnostic data keeps the existing source-generation admission behavior.
 const auto beforeDiagnosticReads=reads;
 auto view=runtime_weather_view();assert(view.generation==runtime_config().generation && !view.configured && !view.weather.data);
 Weather value;value.temperature=29.6;value.humidity=76;value.hourlyCount=4;
 runtime_weather({value,{}},view.generation);view=runtime_weather_view();assert(view.weather.data && view.weather.data->temperature==29.6);
 runtime_weather({std::nullopt,"json"},view.generation);assert(runtime_weather_view().weather.stale);
 runtime_weather({value,{}},view.generation+1);assert(runtime_weather_view().weather.stale);
 assert(reads==beforeDiagnosticReads && configBlob==original && blobWrites==0);
 assert(completeSerialization(runtime_config().config,original) && parseStoredConfig(original).value);
 std::cout<<"Production runtime: packed scalar single-write, authoritative key ignores invalid legacy, type mismatch defaults Thai/no write, read errors remain fatal, reload, open/set/commit failure, coherent commit-before-publish, no tick NVS reads, config/secrets/calibration preserved\n";
}
