#include "../firmware/main/weather_diagnostics.hpp"
#include "esp_tls.h"
#include "esp_heap_caps.h"
#include <cassert>
#include <iostream>
int captures=0;
esp_err_t esp_tls_get_error_handle(esp_tls_t* tls,esp_tls_error_handle_t* out){assert(tls && tls->alive);*out=&tls->errors;return ESP_OK;}
esp_err_t esp_tls_get_and_clear_last_error(esp_tls_error_handle_t errors,int* code,int* flags){++captures;const int result=errors->esp;*code=errors->tls;*flags=errors->flags;*errors={0,0,0};return result;}
size_t heap_caps_get_free_size(unsigned flags){assert(flags==(MALLOC_CAP_INTERNAL|MALLOC_CAP_8BIT));return 1234;}
size_t heap_caps_get_largest_free_block(unsigned flags){assert(flags==(MALLOC_CAP_INTERNAL|MALLOC_CAP_8BIT));return 512;}
#include "../firmware/main/weather_transport_diagnostics.hpp"
int main(){
 using namespace deskduo;
 WeatherDiagnostics collector;auto record=collector.begin(1,100,true);esp_tls_t tls;
 tls.errors={0x801a,-0x2700,8};
 recordWeatherTransportFailure(&record,&tls,WeatherOutcome::TlsFailure,WeatherPhase::Tls,110);
 tls.alive=false; // subsequent SDK/HTTP cleanup destroys the handle
 collector.finish(record,200,1100);const auto result=collector.snapshot();
 assert(result.outcome==WeatherOutcome::TlsFailure && result.phase==WeatherPhase::Tls);
 assert(result.espError==0x801a && result.tlsCode==-0x2700 && result.verifyFlags==8 && result.socketErrno==110);
 assert(result.failureHeap.freeBytes==1234 && result.failureHeap.largestBlock==512 && captures==1);
 recordWeatherTlsErrors(&record,&tls,0);assert(record.tlsCode==-0x2700 && captures==1); // cleanup cannot clear saved codes
 recordWeatherTransportFailure(&record,&tls,WeatherOutcome::Deadline,WeatherPhase::Http,0);assert(captures==1); // first failure retained; no freed-handle read
 record=collector.begin(2,300,true);tls.alive=true;tls.errors={ESP_ERR_ESP_TLS_FAILED_CONNECT_TO_HOST,0,0};
 recordWeatherTransportFailure(&record,&tls,WeatherOutcome::TlsFailure,WeatherPhase::Tls,111);assert(record.outcome==WeatherOutcome::TcpFailure && record.phase==WeatherPhase::Tcp);
 record=collector.begin(3,400,true);recordWeatherTransportFailure(&record,nullptr,WeatherOutcome::Deadline,WeatherPhase::Dns,110);
 assert(!record.espError && !record.tlsCode && !record.verifyFlags && record.socketErrno==110);
 record=collector.begin(4,500,true);tls.errors={0,0,0};recordWeatherTlsErrors(&record,&tls,0);
 assert(record.espError==0 && record.tlsCode==0 && record.verifyFlags==0 && record.socketErrno==0);
 const auto before=captures;recordWeatherTransportFailure(nullptr,&tls,WeatherOutcome::TlsFailure,WeatherPhase::Tls,0);assert(captures==before);
 std::cout<<"Production TLS capture: numeric errors before destroy, first failure retained, TCP classification, unknown handle and internal 8-bit heap passed\n";
}
