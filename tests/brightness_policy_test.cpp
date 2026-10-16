#include "../firmware/main/brightness_policy.hpp"
#include "../firmware/main/runtime.hpp"
#include <cassert>
#include <cstdlib>
#include <iostream>
#include <limits>
using namespace deskduo;
int main(){
 struct TimeCase{uint16_t minute;const char* expected;};
 for(const auto& value:{TimeCase{0,"00:00"},{59,"00:59"},{60,"01:00"},{359,"05:59"},{360,"06:00"},{435,"07:15"},{1080,"18:00"},{1439,"23:59"}})assert(brightness_time_string(value.minute)==value.expected);
 setenv("TZ","ICT-7",1);tzset();
 auto epoch=[](int minute){ std::tm local{};local.tm_year=126;local.tm_mon=9;local.tm_mday=16;local.tm_hour=minute/60;local.tm_min=minute%60;return std::mktime(&local); };
 DisplaySettings s;s.brightnessPercent=11;
 assert(brightness_plan(s,0).percent==11 && brightness_plan(s,epoch(0)).phase==BrightnessPhase::Manual);
 s.brightnessAuto=true;
 assert(brightness_plan(s,std::numeric_limits<std::time_t>::max()).phase==BrightnessPhase::ClockWait);
 assert(brightness_plan(s,1700000000).phase!=BrightnessPhase::ClockWait);
 assert(brightness_plan(s,1699999999).percent==11 && brightness_plan(s,0).phase==BrightnessPhase::ClockWait);
 for(int minute:{0,359,1080,1439})assert(brightness_plan(s,epoch(minute)).percent==6 && brightness_plan(s,epoch(minute)).phase==BrightnessPhase::Night);
 for(int minute:{360,361,1079})assert(brightness_plan(s,epoch(minute)).percent==11 && brightness_plan(s,epoch(minute)).phase==BrightnessPhase::Day);
 for(int base:{5,10,11,99,100}){s.brightnessPercent=base;assert(brightness_plan(s,epoch(1080)).percent==std::max(5,(base+1)/2));}
 s.brightnessDayStart=1185;s.brightnessNightStart=435;
 for(int minute:{1185,1439,0,434})assert(brightness_plan(s,epoch(minute)).phase==BrightnessPhase::Day);
 for(int minute:{435,1184})assert(brightness_plan(s,epoch(minute)).phase==BrightnessPhase::Night);
 DisplayPreference pref;int writes=0;assert(pref.patch(R"({"brightnessPercent":11,"brightnessAuto":true})",[&](const auto&){++writes;return true;})==204);
 for(int minute:{0,360,1080,359,1439,0,360})for(int i=0;i<100;++i){auto plan=brightness_plan(pref.settings(),epoch(minute));assert(plan.percent==(minute>=360 && minute<1080?11:6));}
 assert(brightness_plan(pref.settings(),0).phase==BrightnessPhase::ClockWait && brightness_plan(pref.settings(),epoch(360)).phase==BrightnessPhase::Day);
 assert(pref.settings().brightnessPercent==11 && writes==1);
 std::cout<<"Production policy: process timezone, readiness, boundaries, minute schedules, wrap, rounding, clock jumps/NTP recovery, unchanged base/no writes passed\n";
}
