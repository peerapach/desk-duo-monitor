#include "../firmware/main/core.hpp"
#include "cJSON.h"
#include <cassert>
#include <iostream>
#include "../firmware/main/weather_diagnostics.hpp"
#include <thread>
#include <atomic>
#include <type_traits>
using namespace deskduo;
int main() {
 static_assert(std::is_trivially_copyable<WeatherAttempt>::value, "record must not retain text/dynamic data");
 static_assert(sizeof(WeatherAttempt)<=256,"bounded attempt");
 WeatherDiagnostics collector;
 WeatherView current;current.generation=9;current.configured=true;
 auto json=weatherDiagnosticsJson(collector.snapshot(),current,true,1700000001,100);
 auto* root=cJSON_Parse(json.c_str());assert(root);
 auto* attempt=cJSON_GetObjectItemCaseSensitive(root,"attempt");
 assert(cJSON_GetArraySize(root)==7 && cJSON_GetArraySize(attempt)==22);
 assert(cJSON_IsNull(cJSON_GetObjectItemCaseSensitive(attempt,"httpResult")));
 assert(cJSON_IsNull(cJSON_GetObjectItemCaseSensitive(attempt,"httpStatus")));
 assert(std::string(cJSON_GetObjectItemCaseSensitive(attempt,"outcome")->valuestring)=="not_attempted");
 auto* weather=cJSON_GetObjectItemCaseSensitive(root,"weather");
 assert(cJSON_GetArraySize(weather)==8);
 assert(cJSON_IsFalse(cJSON_GetObjectItemCaseSensitive(weather,"hasData")));
 assert(cJSON_IsNull(cJSON_GetObjectItemCaseSensitive(weather,"ageMs")));cJSON_Delete(root);
 auto record=collector.begin(9,100,false);
 assert(record.attempted && record.outcome==WeatherOutcome::Running);
 record.fail(WeatherOutcome::TlsFailure,WeatherPhase::Tls);
 record.httpResult=-1;record.espError=123;record.tlsCode=-321;record.verifyFlags=8;record.socketErrno=110;
 record.failureHeap={1234,456};
 record.fail(WeatherOutcome::HttpPerform,WeatherPhase::Http); // don't erase actual producer failure
 collector.finish(record,140,90140);
 current.weather.data=Weather{};current.weather.data->temperature=29.6;current.weather.data->humidity=76;current.weather.data->weatherCode=3;current.weather.data->hourlyCount=4;
 current.weather.receivedAtMs=125;current.weather.unknown=false;current.weather.stale=true;
 json=weatherDiagnosticsJson(collector.snapshot(),current,true,1700000001,200);
 root=cJSON_Parse(json.c_str());attempt=cJSON_GetObjectItemCaseSensitive(root,"attempt");weather=cJSON_GetObjectItemCaseSensitive(root,"weather");
 assert(std::string(cJSON_GetObjectItemCaseSensitive(attempt,"outcome")->valuestring)=="tls_failure");
 assert(cJSON_GetObjectItemCaseSensitive(attempt,"elapsedMs")->valuedouble==40);
 assert(cJSON_GetObjectItemCaseSensitive(attempt,"tlsCode")->valueint==-321);
 assert(cJSON_GetObjectItemCaseSensitive(weather,"ageMs")->valueint==75);
 assert(cJSON_GetObjectItemCaseSensitive(weather,"temperature")->valuedouble==29.6);
 assert(cJSON_GetObjectItemCaseSensitive(weather,"hourlyCount")->valueint==4);
 assert(cJSON_IsTrue(cJSON_GetObjectItemCaseSensitive(attempt,"currentGeneration")));cJSON_Delete(root);
 current.generation=10;json=weatherDiagnosticsJson(collector.snapshot(),current,false,1,200);
 root=cJSON_Parse(json.c_str());attempt=cJSON_GetObjectItemCaseSensitive(root,"attempt");assert(cJSON_IsFalse(cJSON_GetObjectItemCaseSensitive(attempt,"currentGeneration")));
 assert(cJSON_IsFalse(cJSON_GetObjectItemCaseSensitive(root,"clockReady")));assert(cJSON_IsFalse(cJSON_GetObjectItemCaseSensitive(root,"stationOnline")));cJSON_Delete(root);
 for(const auto& entry: {std::pair<const char*,const char*>{"no_readings","no_readings"},{"upstream_error","upstream_error"},{"temperature","temperature"},{"secret URL https://user:pass@host?token=secret","other"}}){
 record=collector.begin(10,200,true);record.parserReason=weatherParserReason(entry.first);record.fail(WeatherOutcome::ParseFailure,WeatherPhase::Parse);collector.finish(record,210,90210);
 json=weatherDiagnosticsJson(collector.snapshot(),current,true,1700000001,210);root=cJSON_Parse(json.c_str());attempt=cJSON_GetObjectItemCaseSensitive(root,"attempt");assert(std::string(cJSON_GetObjectItemCaseSensitive(attempt,"parserReason")->valuestring)==entry.second);assert(json.find("secret")==std::string::npos);cJSON_Delete(root);
 }
 record=collector.begin(10,300,true);assert(!record.httpResult && !record.tlsCode && !record.failureHeap.freeBytes && record.parserReason==WeatherParserReason::None);
 unsigned checked=0;
 std::atomic<bool> terminalPublished{false},terminalChecked{false},stop{false};
 std::thread writer([&]{
  for(uint64_t i=1;i<20000;++i){
   auto r=collector.begin(i,i,true);
   r.outcome=WeatherOutcome::Accepted;r.phase=WeatherPhase::Complete;
   collector.finish(r,i+3,i+5);
   if(i==1){
    // Keep one terminal publication available until the reader checks it;
    // then let subsequent publications race with the snapshot reader.
    terminalPublished=true;
    while(!terminalChecked)std::this_thread::yield();
   }
  }
  stop=true;
 });
 const auto checkTerminal=[&](const WeatherAttempt& r){
  assert(r.outcome==WeatherOutcome::Accepted && r.phase==WeatherPhase::Complete);
  assert(r.elapsedMs==3 && r.nextDueMs==r.startedMs+5 && r.generation==r.startedMs);
  ++checked;
 };
 while(!terminalPublished)std::this_thread::yield();
 checkTerminal(collector.snapshot());terminalChecked=true;
 while(!stop){
  const auto r=collector.snapshot();
  if(r.outcome!=WeatherOutcome::Running)checkTerminal(r);
 }
 writer.join();
 assert(checked>0 && "concurrent reader must examine a terminal snapshot");
 std::cout<<"Weather diagnostics bounded record, unknown values, first failure, privacy, accepted snapshot, generation and coherent reader passed; terminal snapshots checked: "<<checked<<"\n";
}
