#include "../firmware/main/touch_diagnostics.hpp"
#include "cJSON.h"
#include <cassert>
#include <iostream>
int main() {
    deskduo::TouchDiagnostics history;
    auto initial=history.snapshot();
    assert(initial.sequence==0 && initial.count==0 && !initial.pressureAccepted);
    history.poll(9,80,false);
    auto released=history.snapshot();
    assert(released.sequence==1 && released.lastPressure==80 && released.lastPollTimeMs==9 && !released.pressureAccepted && released.count==0);
    for(int i=0;i<40;++i) history.poll(100+i,200,true,1000+i,2000+i,10+i,20+i);
    auto held=history.snapshot();
    assert(held.sequence==41 && held.count==32 && held.pressureAccepted);
    assert(held.samples[0].sequence==10 && held.samples[0].timeMs==108);
    assert(held.samples[0].rawX==1008 && held.samples[0].rawY==2008 && held.samples[0].x==18 && held.samples[0].y==28 && held.samples[0].pressure==200);
    assert(held.samples[31].sequence==41 && held.samples[31].rawX==1039);
    history.poll(150,0,false);
    history.poll(151,3001,false);
    auto after=history.snapshot();
    assert(after.sequence==43 && after.count==32 && after.lastPressure==3001 && after.lastPollTimeMs==151 && !after.pressureAccepted);
    assert(after.samples[0].sequence==10 && after.samples[31].sequence==41);
    history.poll(152,100,true,3000,4000,239,319);
    auto next=history.snapshot();
    assert(next.count==32 && next.samples[0].sequence==11 && next.samples[31].sequence==44);
    assert(held.samples[0].sequence==10 && held.sequence==41); // copied snapshots stay immutable
    auto encoded=deskduo::touchDiagnosticsJson(after);
    auto* json=cJSON_Parse(encoded.c_str());assert(json);
    assert(cJSON_GetObjectItemCaseSensitive(json,"sequence")->valuedouble==43);
    assert(cJSON_GetObjectItemCaseSensitive(json,"lastPressure")->valueint==3001);
    assert(cJSON_GetObjectItemCaseSensitive(json,"lastPollTimeMs")->valuedouble==151);
    assert(cJSON_IsFalse(cJSON_GetObjectItemCaseSensitive(json,"pressureAccepted")));
    auto* samples=cJSON_GetObjectItemCaseSensitive(json,"samples");assert(cJSON_GetArraySize(samples)==32);
    auto* first=cJSON_GetArrayItem(samples,0);auto* last=cJSON_GetArrayItem(samples,31);
    assert(cJSON_GetObjectItemCaseSensitive(first,"rawX")->valueint==1008);
    assert(cJSON_GetObjectItemCaseSensitive(first,"rawY")->valueint==2008);
    assert(cJSON_GetObjectItemCaseSensitive(first,"pressure")->valueint==200);
    assert(cJSON_GetObjectItemCaseSensitive(first,"x")->valueint==18);
    assert(cJSON_GetObjectItemCaseSensitive(first,"y")->valueint==28);
    assert(cJSON_GetObjectItemCaseSensitive(last,"timeMs")->valuedouble==139);
    assert(cJSON_GetObjectItemCaseSensitive(last,"sequence")->valuedouble==41);
    cJSON_Delete(json);
    std::cout<<"Touch diagnostics bounded chronology, release retention, immutable snapshot and JSON checks passed\n";
}
