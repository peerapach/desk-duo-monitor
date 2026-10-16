#include "lvgl.h"
#include "src/misc/lv_text_private.h"
#include "ui.hpp"
#include "runtime.hpp"
#include "assets.hpp"
#include "backlight.hpp"
#include "ota_policy.hpp"
#include "fonts/ct_font_thai.h"
#include <cassert>
#include <fstream>
#include <iostream>
#include <cstring>
#include <cstdlib>
#include <ctime>
#include <array>
#include <limits>
int buttonLevel=1;
uint64_t clockMs=2000;
namespace deskduo {
Config cfg;
Snapshot data;
CodexUsageMode mode=CodexUsageMode::Both;
Language language=Language::Thai;
uint8_t brightness=100, hardwareBrightness=100;
unsigned hardwareCalls=0, admissions=0, persistence=0;
bool hardwareFail=false, saveFail=false;
std::vector<unsigned> hardwareLevels;
esp_err_t display_set_brightness(uint8_t level){++hardwareCalls;hardwareLevels.push_back(level);if(hardwareFail)return ESP_FAIL;hardwareBrightness=level;return ESP_OK;}
int runtime_display_patch(const std::string& body){
 ++admissions;assert(!maintenance.acquire(Maintenance::Settings) && "local save must hold shared Settings lease");
 assert(body.find("language")==std::string::npos && body.find("codexUsageMode")==std::string::npos);
 if(saveFail)return 503;
 auto start=body.find(':');assert(start!=std::string::npos);brightness=std::stoi(body.substr(start+1));++persistence;return 204;
}
Language runtime_language(){return language;}
DisplaySettings runtime_display_settings(){return {mode,language,brightness};}
CodexUsageMode runtime_display_mode(){return mode;}
uint64_t now_ms(){return clockMs;}
RequestConfig runtime_config(){return {cfg,0};}
Snapshot runtime_snapshot(){return data;}
std::string ap_name(){return "DeskDuo-DEMO01";}
std::string station_address(){return "192.0.2.10";}
}
uint8_t frame[240*320*3]{};
alignas(LV_DRAW_BUF_ALIGN) uint8_t buffer[240*24*3];
void capture(const char* name){
    deskduo::ui_tick();lv_refr_now(nullptr);
    std::ofstream f(name,std::ios::binary);f<<"P6\n240 320\n255\n";f.write((char*)frame,sizeof(frame));
}
// Inspect actual LVGL labels without relying on private child indexes.
lv_obj_t* findLabel(lv_obj_t* parent,const char* text){
    if(lv_obj_check_type(parent,&lv_label_class) && std::strcmp(lv_label_get_text(parent),text)==0)return parent;
    for(uint32_t i=0;i<lv_obj_get_child_count(parent);++i)
        if(auto* match=findLabel(lv_obj_get_child(parent,i),text))return match;
    return nullptr;
}
// Locate the rendered primary bar by its relation to the selected reset label,
// without depending on private UI child indexes.
lv_obj_t* findBarBelow(lv_obj_t* reset){
    lv_obj_t* nearest=nullptr;
    auto* parent=lv_obj_get_parent(reset);
    for(uint32_t i=0;i<lv_obj_get_child_count(parent);++i){
        auto* child=lv_obj_get_child(parent,i);
        if(lv_obj_check_type(child,&lv_bar_class) && lv_obj_get_y(child)>lv_obj_get_y(reset) &&
           (!nearest || lv_obj_get_y(child)<lv_obj_get_y(nearest)))nearest=child;
    }
    return nearest;
}
void tapTab(const char* name){
    const char* translated=name;
    if(deskduo::language==deskduo::Language::English) {
        if(std::strcmp(name,"คู่หู")==0)translated="Duo";
        if(std::strcmp(name,"อากาศ")==0)translated="Weather";
        if(std::strcmp(name,"หุ้น")==0)translated="Stocks";
        if(std::strcmp(name,"ตั้งค่า")==0)translated="Setup";
    }
    auto shaped=deskduo::shapeText(translated);
    auto* tab=findLabel(lv_screen_active(),shaped.c_str());
    assert(tab && "all pages must be reachable by a visible bottom button");
    lv_obj_send_event(lv_obj_get_parent(tab),LV_EVENT_CLICKED,nullptr);
    deskduo::ui_tick();
    auto* ip=findLabel(lv_screen_active(),"192.0.2.10");
    assert(ip && "LAN IP must remain visible on every page");
    assert(!lv_obj_has_flag(ip,LV_OBJ_FLAG_CLICKABLE));
    lv_obj_update_layout(lv_screen_active());
    lv_area_t navArea,ipArea;
    lv_obj_get_coords(lv_obj_get_parent(tab),&navArea);lv_obj_get_coords(ip,&ipArea);
    assert(navArea.y1==276 && navArea.y2==319 && navArea.x2-navArea.x1+1==60);
    assert(ipArea.y1>=256 && ipArea.y2<276);
}
int pointerX=0, pointerY=0;
lv_indev_state_t pointerState=LV_INDEV_STATE_RELEASED;
lv_indev_t* fakePointer=nullptr;
void pointerTapAt(int x,int y){
    pointerX=x; pointerY=y; pointerState=LV_INDEV_STATE_PRESSED; lv_indev_read(fakePointer);
    pointerState=LV_INDEV_STATE_RELEASED; lv_indev_read(fakePointer);
    deskduo::ui_tick();
}

lv_obj_t* findSlider(lv_obj_t* parent){
 if(lv_obj_check_type(parent,&lv_slider_class))return parent;
 for(uint32_t i=0;i<lv_obj_get_child_count(parent);++i)if(auto* match=findSlider(lv_obj_get_child(parent,i)))return match;
 return nullptr;
}
void pointerAt(int x,int y,bool pressed){pointerX=x;pointerY=y;pointerState=pressed?LV_INDEV_STATE_PRESSED:LV_INDEV_STATE_RELEASED;clockMs+=20;lv_indev_read(fakePointer);if(pressed){clockMs+=20;lv_indev_read(fakePointer);}deskduo::ui_tick();}
void brightnessChecks(){
 using namespace deskduo;
 language=Language::English;brightness=50;ui_tick();tapTab("ตั้งค่า");
 auto slider=[](){auto* s=findSlider(lv_screen_active());assert(s && "Setup needs real brightness slider");return s;};
 auto* s=slider();lv_obj_update_layout(s);lv_area_t b{};lv_obj_get_coords(s,&b);
 assert(b.y1-20>=24 && b.y2+20<180 && b.y2-b.y1+1+40>=44);
 assert(hardwareBrightness==50);auto calls=hardwareCalls;ui_tick();ui_tick();assert(hardwareCalls==calls);
 const int y=(b.y1+b.y2)/2;
 auto press=[&](int x,int yy){pointerAt(x,yy,true);};auto release=[&](int x,int yy){pointerAt(x,yy,false);};
 auto drag=[&](int x){press((b.x1+b.x2)/2,y);press(x,y);};
 auto saved=persistence;auto admitted=admissions;
 // Both halves of the padded target belong to the slider, including vertical
 // motion outside its thin track; the scrollable credential viewport stays put.
 press((b.x1+b.x2)/2,y-18);press(b.x1,y);press(b.x1,y+60);
 assert(hardwareBrightness==5 && persistence==saved && admissions==admitted && lv_obj_get_scroll_y(lv_obj_get_parent(s))==0);
 release(b.x1,y+60);assert(persistence==saved+1 && admissions==admitted+1);
 saved=persistence;admitted=admissions;
 press(b.x1,y+18);assert(persistence==saved);press((b.x1+b.x2)/2,y);assert(persistence==saved);assert(hardwareBrightness>=49 && hardwareBrightness<=54);
 press(b.x2,y);assert(hardwareBrightness==100 && persistence==saved);press(b.x1,y);assert(hardwareBrightness==5 && persistence==saved);
 assert(findLabel(lv_screen_active(),"Desk Duo setup"));assert(lv_obj_get_scroll_y(lv_obj_get_parent(s))==0);
 release(b.x1,y);assert(persistence==saved+1 && admissions==admitted+1 && brightness==5);
 capture("brightness-setup-en.ppm");
 // Saved web changes reach the same hardware without any navigation rebuild.
 auto* existing=slider();brightness=50;ui_tick();assert(slider()==existing && hardwareBrightness==50 && lv_slider_get_value(existing)==50);
 // A drag must not be overwritten by web updates; release merges only brightness.
 drag(b.x2);assert(lv_slider_get_value(slider())==100 && hardwareBrightness==100 && persistence==saved+1 && admissions==admitted+1);brightness=25;mode=CodexUsageMode::Weekly;ui_tick();assert(hardwareBrightness==100);release(b.x2,y);assert(brightness==100 && mode==CodexUsageMode::Weekly);
 saved=persistence;admitted=admissions;
 unsigned resetEvents=0,lostEvents=0;
 lv_obj_add_event_cb(slider(),[](lv_event_t* e){++*static_cast<unsigned*>(lv_event_get_user_data(e));},LV_EVENT_INDEV_RESET,&resetEvents);
 lv_obj_add_event_cb(slider(),[](lv_event_t* e){++*static_cast<unsigned*>(lv_event_get_user_data(e));},LV_EVENT_PRESS_LOST,&lostEvents);
 drag(b.x1);assert(hardwareBrightness==5);brightness=50;lv_indev_reset(fakePointer,slider());release(b.x1,y);
 assert(resetEvents==1 && lostEvents==0 && persistence==saved && admissions==admitted && hardwareBrightness==50 && lv_slider_get_value(slider())==50);
 drag(b.x1);assert(hardwareBrightness==5);brightness=75;lv_indev_wait_release(fakePointer);release(b.x1,y);
 assert(lostEvents==1 && persistence==saved && admissions==admitted && hardwareBrightness==75 && lv_slider_get_value(slider())==75);
 brightness=50;ui_tick();assert(hardwareBrightness==50 && lv_slider_get_value(slider())==50);
 drag(b.x1);assert(hardwareBrightness==5);tapTab("คู่หู");release(b.x1,y);assert(persistence==saved && hardwareBrightness==50 && !findSlider(lv_screen_active()));
 tapTab("ตั้งค่า");s=slider();drag(b.x1);assert(hardwareBrightness==5);language=Language::Thai;ui_tick();release(b.x1,y);assert(persistence==saved && hardwareBrightness==50);capture("brightness-setup-th.ppm");
 language=Language::English;ui_tick();drag(b.x1);assert(hardwareBrightness==5);buttonLevel=0;ui_tick();clockMs+=100;buttonLevel=1;ui_tick();release(b.x1,y);assert(persistence==saved && hardwareBrightness==50 && !findSlider(lv_screen_active()));
 tapTab("ตั้งค่า");assert(maintenance.acquire(Maintenance::Update));admitted=admissions;drag(b.x1);release(b.x1,y);maintenance.release();assert(persistence==saved && admissions==admitted && hardwareBrightness==50);assert(findLabel(lv_screen_active(),"Could not save"));
 saveFail=true;admitted=admissions;drag(b.x1);release(b.x1,y);saveFail=false;assert(admissions==admitted+1 && persistence==saved && hardwareBrightness==50);assert(findLabel(lv_screen_active(),"Could not save"));
 hardwareFail=true;admitted=admissions;drag(b.x1);release(b.x1,y);hardwareFail=false;ui_tick();assert(persistence==saved && admissions==admitted && hardwareBrightness==50);assert(findLabel(lv_screen_active(),"Brightness failed"));
 // Failed committed apply is retried; successful applications alone are cached.
 brightness=25;hardwareFail=true;calls=hardwareCalls;ui_tick();assert(hardwareCalls>calls && hardwareBrightness==50);hardwareFail=false;ui_tick();assert(hardwareBrightness==25);
 // Bounded repeated rebuilds while previewing cannot retain widgets or persist.
 lv_mem_monitor_t before{},after{};lv_mem_monitor(&before);
 for(int i=0;i<40;++i){drag(b.x1);language=i%2?Language::English:Language::Thai;ui_tick();release(b.x1,y);tapTab("คู่หู");tapTab("ตั้งค่า");}
 lv_mem_monitor(&after);assert(after.free_size+512>=before.free_size);assert(persistence==saved);
 brightness=100;language=Language::Thai;ui_tick();tapTab("คู่หู");
 std::cout<<"Brightness actual pointer: padding/min/mid/max, preview/no writes, one leased release, sparse merge, cancel/language/BOOT, busy/save/apply failure, web sync, 40 rebuilds passed\n";
}

// These tests catch fabricated missing readings, wrong WMO branches, text
// clipping, misplaced chart annotations, and stale pointers after rebuilds.
lv_obj_t* weatherLabel(const char* value){
    auto* obj=findLabel(lv_screen_active(),deskduo::shapeText(value).c_str());
    if(!obj)std::cerr<<"Missing weather label: "<<value<<"\n";
    assert(obj && "weather reading must appear on actual LVGL page");
    return obj;
}
void assertTextFits(lv_obj_t* obj){
    lv_point_t natural{};
    lv_text_get_size(&natural,lv_label_get_text(obj),lv_obj_get_style_text_font(obj,0),
                     lv_obj_get_style_text_letter_space(obj,0),0,10000,LV_TEXT_FLAG_NONE);
    if(natural.x>lv_obj_get_width(obj))std::cerr<<"Clipped label: "<<lv_label_get_text(obj)<<" width="<<natural.x<<" reserved="<<lv_obj_get_width(obj)<<"\n";
    assert(natural.x<=lv_obj_get_width(obj) && "weather value must fit its actual native font bounds");
    assert(natural.y<=lv_obj_get_style_text_font(obj,0)->line_height &&
           lv_obj_get_height(obj)<=lv_obj_get_style_text_font(obj,0)->line_height && "weather reading must stay on one baseline");
    lv_area_t box{};lv_obj_get_coords(obj,&box);
    assert(box.x1>=0 && box.x2<240 && box.y1>=24 && box.y2<256);
}
// Thai fallback has a taller line box. Verify actual glyph ink stays above
// its usage bar, rather than treating a tall label box as harmless.
void assertGlyphsAboveBar(lv_obj_t* label){
    auto* bar=findBarBelow(label);assert(bar);
    const auto* font=lv_obj_get_style_text_font(label,0);
    const auto* text=lv_label_get_text(label);uint32_t offset=0;
    int bottom=0;
    while(text[offset]) {
        const auto cp=lv_text_encoded_next(text,&offset);
        lv_font_glyph_dsc_t glyph{};assert(lv_font_get_glyph_dsc(font,&glyph,cp,0));
        if(glyph.box_h)bottom=std::max(bottom,static_cast<int>(font->line_height-font->base_line-glyph.ofs_y-1));
    }
    if(lv_obj_get_y(label)+bottom>=lv_obj_get_y(bar))std::cerr<<"Overlap "<<lv_label_get_text(label)<<" y="<<lv_obj_get_y(label)<<" glyphBottom="<<bottom<<" bar="<<lv_obj_get_y(bar)<<" height="<<font->line_height<<" baseline="<<font->base_line<<"\n";
    assert(lv_obj_get_y(label)+font->line_height-font->base_line==lv_obj_get_y(bar)-4 && "reset baseline must stay in its reserved usage row after language rebuild");
    assert(lv_obj_get_y(label)+bottom<lv_obj_get_y(bar) && "translated reset glyphs must not cross the fixed usage bar");
}
uint64_t regionHash(int x,int y,int width,int height){
    uint64_t hash=1469598103934665603ULL;
    for(int j=y;j<y+height;++j)for(int i=x;i<x+width;++i)for(int c=0;c<3;++c)
        hash=(hash^frame[(j*240+i)*3+c])*1099511628211ULL;
    return hash;
}
bool mintNear(int x,int y){
    for(int j=y-2;j<=y+2;++j)for(int i=x-2;i<=x+2;++i){
        auto* p=&frame[(j*240+i)*3];
        if(p[0]>150 && p[1]>180 && p[2]>170 && p[1]>p[0]+10)return true;
    }
    return false;
}
void weatherChecks(){
    auto& data=deskduo::data;
    const auto originalWeather=data.weather;
    deskduo::language=deskduo::Language::Thai;deskduo::ui_tick();
    tapTab("อากาศ");capture("weather.ppm");
    auto* temperature=weatherLabel("31.0");assertTextFits(temperature);
    assert(lv_obj_get_style_text_font(temperature,0)->line_height>=48 && "normal weather must use real large font");
    auto* unit=weatherLabel("°C");assertTextFits(unit);
    lv_area_t tempBox{},unitBox{};lv_obj_get_coords(temperature,&tempBox);lv_obj_get_coords(unit,&unitBox);
    assert(tempBox.x2<unitBox.x1);
    weatherLabel("มีเมฆบางส่วน");weatherLabel("โอกาสฝน");
    auto* content=lv_obj_get_parent(temperature);
    assert(lv_obj_get_y(content)==24 && lv_obj_get_height(content)==232);
    assert(!lv_obj_has_flag(content,LV_OBJ_FLAG_SCROLLABLE));
    const char* times[]={"14:00","15:00","16:00","17:00"};
    const char* readings[]={"32.0°","33.0°","34.0°","35.0°"};
    for(int k=0;k<4;++k){
        auto* t=weatherLabel(times[k]);auto* r=weatherLabel(readings[k]);assertTextFits(t);assertTextFits(r);
        assert(lv_obj_get_x(t)==12+54*k && lv_obj_get_width(t)==54);
        assert(lv_obj_get_x(r)==lv_obj_get_x(t));
        assert(lv_obj_get_y(r)>lv_obj_get_y(t) && lv_obj_get_y(r)<198);
        assert(lv_obj_get_style_text_font(r,0)->line_height>=14);
    }
    // Hand-checked scale for 32,33,34,35: padded range 31.64..35.36,
    // point rows 60,53,47,40 within the plot at screen y=160.
    assert(mintNear(39,220) && mintNear(93,213) && mintNear(147,207) && mintNear(201,200));
    // Supported codes select distinct condition artwork; codes inside arbitrary
    // numeric gaps must remain unknown instead of being classified as fog/rain.
    struct Condition { const char* label; std::initializer_list<int> codes; };
    const Condition groups[]={
        {"ท้องฟ้าแจ่มใส",{0,1}}, {"มีเมฆบางส่วน",{2}}, {"เมฆมาก",{3}},
        {"หมอก",{45,48}}, {"ฝนปรอย",{51,53,55,56,57}},
        {"ฝนตก",{61,63,65,66,67,80,81,82}},
        {"หิมะ",{71,73,75,77,85,86}}, {"พายุฝน",{95,96,97,99}},
        {"สภาพอากาศ --",{-1,4,44,46,49,50,52,58,60,68,70,78,79,83,87,94,98,100}}
    };
    std::array<uint64_t,9> icons{};
    for(unsigned group=0;group<9;++group){
        for(int code:groups[group].codes){
            data.weather.data->weatherCode=code;clockMs+=1000;capture("weather-condition-last.ppm");
            weatherLabel(groups[group].label);
            auto hash=regionHash(162,68,65,65);
            if(!icons[group])icons[group]=hash;
            else assert(hash==icons[group] && "same condition group must keep consistent artwork");
        }
        for(unsigned previous=0;previous<group;++previous)
            assert(icons[group]!=icons[previous] && "weather groups need distinguishable native artwork");
        capture(("weather-condition-"+std::to_string(group)+".ppm").c_str());
    }
    data.weather.data->weatherCode.reset();clockMs+=1000;capture("weather-absent-code.ppm");
    assert(regionHash(162,68,65,65)==icons[8]);
    data.weather.data->weatherCode=63;data.weather.stale=true;
    clockMs+=60000;
    data.weather.receivedAtMs=clockMs-60000;clockMs+=1000;capture("weather-rain-stale.ppm");
    weatherLabel("ข้อมูลเก่า 1 น.");weatherLabel("31.0");
    data.weatherLabel="สถานีอากาศกรุงเทพมหานครชื่อภาษาไทยยาวมาก";
    clockMs+=1000;capture("weather-long-thai.ppm");
    auto* place=weatherLabel(data.weatherLabel.c_str());auto* status=weatherLabel("ข้อมูลเก่า 1 น.");
    lv_area_t placeBox{},statusBox{};lv_obj_get_coords(place,&placeBox);lv_obj_get_coords(status,&statusBox);
    assert(placeBox.x2<statusBox.x1 && "long Thai location must not overlap receipt status");
    assert(lv_label_get_long_mode(place)==LV_LABEL_LONG_DOT || lv_label_get_long_mode(place)==LV_LABEL_LONG_SCROLL_CIRCULAR);
    assertTextFits(status);
    // Receipt ages of two-digit minutes and many hours must remain readable.
    for(uint64_t elapsed:{59ULL*60000,999ULL*3600000,2000ULL*3600000}){
        clockMs=data.weather.receivedAtMs+elapsed;deskduo::ui_tick();lv_obj_update_layout(lv_screen_active());
        assertTextFits(status);
    }
    data.weatherLabel="กรุงเทพฯ";data.weather.stale=false;
    data.weather.data->temperature=0;data.weather.data->humidity=0;
    for(int k=0;k<4;++k){data.weather.data->hourlyTemperatures[k]=0;data.weather.data->hourlyRainProbability[k]=0;}
    clockMs+=1000;capture("weather-flat-zero.ppm");weatherLabel("0.0");weatherLabel("0%");
    // A flat series is centered safely; each slot keeps a real point and time.
    for(int k=0;k<4;++k)assert(mintNear(39+54*k,210));
    for(uint32_t i=0;i<lv_obj_get_child_count(content);++i){
        auto* child=lv_obj_get_child(content,i);
        if(lv_obj_check_type(child,&lv_label_class) && lv_obj_get_y(child)==210)
            assert(std::strcmp(lv_label_get_text(child),"0%")==0 && "all real zero rain probabilities must remain visible");
    }
    data.weather.data->hourlyRainProbability[0].reset();clockMs+=1000;deskduo::ui_tick();
    assert(mintNear(39,210)); // Missing rain never invalidates a known temperature.
    data.weather.data->hourlyCount=3;clockMs+=1000;capture("weather-count-three.ppm");
    assert(!findLabel(lv_screen_active(),"17:00") && !mintNear(201,210));
    data.weather.data->hourlyCount=4;data.weather.data->hourlyTemperatures[1].reset();
    data.weather.data->hourlyTimes[3].reset();
    clockMs+=1000;capture("weather-sparse.ppm");
    assert(!findLabel(lv_screen_active(),"15:00") && findLabel(lv_screen_active(),"16:00") && !findLabel(lv_screen_active(),"17:00"));
    assert(mintNear(39,210) && mintNear(147,210));
    assert(!mintNear(66,210) && !mintNear(120,210) && "sparse plot must not bridge a missing slot");
    assert(!mintNear(93,210) && !mintNear(201,210));
    data.weather.data->hourlyCount=0;clockMs+=1000;capture("weather-count-zero.ppm");
    assert(!findLabel(lv_screen_active(),"14:00") && !findLabel(lv_screen_active(),"16:00"));
    assert(!mintNear(39,210) && !mintNear(147,210));
    data.weather.unknown=true;clockMs+=1000;capture("weather-unknown.ppm");
    weatherLabel("ยังไม่มีข้อมูล");assert(!findLabel(lv_screen_active(),"0.0"));
    assert(!findLabel(lv_screen_active(),"14:00"));assert(regionHash(162,68,65,65)==icons[8]);
    const auto retained=data.weather.data;data.weather.data.reset();data.weather.unknown=false;
    clockMs+=1000;capture("weather-no-data.ppm");weatherLabel("ยังไม่มีข้อมูล");
    assert(regionHash(162,68,65,65)==icons[8]);data.weather.data=retained;
    data.weather.unknown=false;data.weather.data->hourlyCount=4;
    for(int k=0;k<4;++k){data.weather.data->hourlyTimes[k]=1791097200+k*3600;data.weather.data->hourlyTemperatures[k]=-99.9+k;}
    for(double value:{-99.9,999.9,-999.9,999.99,-999.99,1e300,-1e300,std::numeric_limits<double>::infinity()}){
        data.weather.data->temperature=value;clockMs+=1000;capture("weather-extreme-last.ppm");
        const char* expected=value==-99.9?"-99.9":value==999.9?"999.9":value==-999.9?"-999.9":value==999.99?"1000.0":value==-999.99?"-1000.0":"เกินช่วง";
        assertTextFits(weatherLabel(expected));
        for(int k=0;k<4;++k)assertTextFits(weatherLabel((std::string(k==0?"-99.9":k==1?"-98.9":k==2?"-97.9":"-96.9")+"°").c_str()));
        if(value==-999.9)capture("weather-extremes.ppm");
    }
    data.weather.data->hourlyTemperatures={-999.99,999.99,-999.9,999.9};
    clockMs+=1000;capture("weather-forecast-extremes.ppm");
    const char* wideReadings[]={"-1000.0°","1000.0°","-999.9°","999.9°"};
    for(int k=0;k<4;++k){
        auto* reading=weatherLabel(wideReadings[k]);assertTextFits(reading);
        assert(lv_obj_get_x(reading)==12+54*k && lv_obj_get_width(reading)==54);
        assert(lv_obj_get_style_text_font(reading,0)->line_height>=12);
    }
    // Rebuild pages repeatedly after edge cases, then update current weather.
    lv_mem_monitor_t before{},after{};lv_mem_monitor(&before);
    for(int i=0;i<40;++i){tapTab("คู่หู");tapTab("หุ้น");tapTab("อากาศ");}
    lv_mem_monitor(&after);assert(after.free_size+512>=before.free_size);
    std::cout<<"Weather LVGL heap after 40 page rebuild cycles: "<<after.total_size-after.free_size<<" bytes; free "<<after.free_size<<" bytes\n";
    data.weather=originalWeather;
    clockMs+=1000;capture("weather-navigation-cycled.ppm");weatherLabel("31.0");
    auto* currentContent=lv_obj_get_parent(weatherLabel("31.0"));
    // The device stays in Asia/Bangkok throughout. Hand-written local hours
    // correspond to each normalized UTC epoch plus the source's own offset.
    // These cases catch accidental use of localtime_r and treating offset0
    // as missing, with both previous-day and next-day midnight rollover.
    struct TimezoneCase { int64_t epoch; int32_t offset; const char* name; std::array<const char*,4> times; };
    const TimezoneCase timezoneCases[]={
        {1791153900,19800,"positive",{"04:15","05:15","06:15","07:15"}},
        {1791162000,-18000,"negative",{"20:00","21:00","22:00","23:00"}},
        {1791154800,0,"utc",{"23:00","00:00","01:00","02:00"}},
        {1791162000,-12600,"negative-fractional",{"21:30","22:30","23:30","00:30"}}
    };
    data.weatherLabel="สถานีทดสอบ";
    for(const auto& fixture:timezoneCases){
        data.weather.data->utcOffsetSeconds=fixture.offset;
        for(int k=0;k<4;++k)data.weather.data->hourlyTimes[k]=fixture.epoch+k*3600;
        clockMs+=1000;capture((std::string("weather-timezone-")+fixture.name+".ppm").c_str());
        for(int k=0;k<4;++k){
            auto* hour=weatherLabel(fixture.times[k]);assertTextFits(hour);
            assert(lv_obj_get_x(hour)==12+54*k && lv_obj_get_y(hour)==136);
            assert(lv_obj_get_x(weatherLabel(readings[k]))==lv_obj_get_x(hour));
        }
        // The source formatter must not mutate TZ or the device clock zone.
        time_t epoch=fixture.epoch;struct tm device{};assert(localtime_r(&epoch,&device));
        assert(device.tm_hour==static_cast<int>((fixture.epoch/3600+7)%24));
    }
    data.weather.data->hourlyTimes[1].reset();
    clockMs+=1000;capture("weather-timezone-missing-timestamp.ppm");
    assert(!findLabel(lv_screen_active(),"22:30"));
    assert(!mintNear(93,213));
    data.weather.data->hourlyTimes[1]=1791165600;
    data.weather.data->utcOffsetSeconds.reset();
    clockMs+=1000;capture("weather-timezone-missing-offset.ppm");
    unsigned unknownHours=0;
    for(uint32_t i=0;i<lv_obj_get_child_count(currentContent);++i){
        auto* child=lv_obj_get_child(currentContent,i);
        if(lv_obj_check_type(child,&lv_label_class) && lv_obj_get_y(child)==136){
            assert(std::strcmp(lv_label_get_text(child),"--")==0 && "source-local time without a source offset must remain unknown");
            ++unknownHours;
        }
    }
    assert(unknownHours==4);
    assert(mintNear(39,220) && mintNear(93,213) && mintNear(147,207) && mintNear(201,200));
    data.weather=originalWeather;data.weatherLabel="กรุงเทพฯ";
    clockMs+=1000;deskduo::ui_tick();
    std::cout<<"Weather source-timezone: positive/negative/fractional/UTC0, midnight rollover, missing timestamp/offset, unchanged Bangkok device zone passed\n";
    std::cout<<"Calm weather: fresh/unknown/sparse/zero/flat/stale, exact WMO groups, native font bounds, long Thai, signed extremes, and 40 rebuild cycles passed\n";
}
// Language changes must rebuild the selected page on the UI task, preserve
// truthful data/window selection, and use the actual embedded font metrics.
void languageChecks(){
    using namespace deskduo;
    auto original=data;
    for(auto selected:{Language::Thai,Language::English}) {
        const bool english=selected==Language::English;
        language=selected;clockMs+=1000;ui_tick();
        const std::string suffix=english?"en":"th";
        tapTab("คู่หู");mode=CodexUsageMode::Weekly;clockMs+=1000;
        capture(("language-usage-weekly-"+suffix+".ppm").c_str());
        assert(findLabel(lv_screen_active(),"21%"));
        auto* reset=findLabel(lv_screen_active(),shapeText(english?"Weekly reset 07/10 14:00":"สัปดาห์ เริ่มใหม่ 07/10 14:00").c_str());
        assert(reset);assertGlyphsAboveBar(reset);
        language=english?Language::Thai:Language::English;clockMs+=20;ui_tick();
        assert(findLabel(lv_screen_active(),"21%"));
        assert(findLabel(lv_screen_active(),shapeText(english?"สัปดาห์ เริ่มใหม่ 07/10 14:00":"Weekly reset 07/10 14:00").c_str()));
        language=selected;clockMs+=20;ui_tick();
        data.claude.stale=true;data.claude.receivedAtMs=clockMs;clockMs+=1000;ui_tick();
        assert(findLabel(lv_screen_active(),shapeText(english?"stale 0m":"เก่า 0น.").c_str()));
        mode=CodexUsageMode::Both;clockMs+=1000;
        capture(("language-usage-both-"+suffix+".ppm").c_str());
        assert(findLabel(lv_screen_active(),"58%"));
        tapTab("อากาศ");capture(("language-weather-"+suffix+".ppm").c_str());
        assertTextFits(weatherLabel(english?"Partly cloudy":"มีเมฆบางส่วน"));
        assertTextFits(weatherLabel(english?"Humidity":"ความชื้น"));
        assertTextFits(weatherLabel(english?"Rain %":"โอกาสฝน"));
        weatherLabel("กรุงเทพฯ"); // User-entered location remains unchanged.
        if(english) {
            struct Condition { int code; const char* text; };
            for(const auto& condition:std::array<Condition,9>{{{0,"Clear"},{2,"Partly cloudy"},{3,"Cloudy"},{45,"Fog"},{51,"Drizzle"},{61,"Rain"},{71,"Snow"},{95,"Thunderstorm"},{4,"Condition --"}}}) {
                data.weather.data->weatherCode=condition.code;clockMs+=1000;ui_tick();lv_obj_update_layout(lv_screen_active());
                assertTextFits(weatherLabel(condition.text));
            }
            data.weather.data->weatherCode=2;clockMs+=1000;ui_tick();
        }
        const auto weather=data.weather;
        data.weather.stale=true;data.weather.receivedAtMs=clockMs-59000;clockMs+=1000;
        capture(("language-weather-stale-"+suffix+".ppm").c_str());
        assertTextFits(weatherLabel(english?"Stale 1m":"ข้อมูลเก่า 1 น."));
        for(uint64_t elapsed:{59ULL*60000,999ULL*3600000,2000ULL*3600000}) {
            clockMs=data.weather.receivedAtMs+elapsed;ui_tick();lv_obj_update_layout(lv_screen_active());
            auto* content=lv_obj_get_parent(weatherLabel("31.0"));
            for(uint32_t k=0;k<lv_obj_get_child_count(content);++k){
                auto* label=lv_obj_get_child(content,k);
                if(lv_obj_check_type(label,&lv_label_class) && lv_obj_get_x(label)==142)assertTextFits(label);
            }
        }
        data.weather.unknown=true;clockMs+=1000;
        capture(("language-weather-unknown-"+suffix+".ppm").c_str());
        assertTextFits(weatherLabel(english?"No data":"ยังไม่มีข้อมูล"));
        assertTextFits(weatherLabel(english?"Condition --":"สภาพอากาศ --"));
        assert(!findLabel(lv_screen_active(),"31.0"));
        data.weather=weather;data.weather.data->temperature=1e300;clockMs+=1000;
        capture(("language-weather-extreme-"+suffix+".ppm").c_str());
        assertTextFits(weatherLabel(english?"Out of range":"เกินช่วง"));
        data.weather=weather;
        // Same-page switch must translate fixed labels and keep the weather view.
        language=english?Language::Thai:Language::English;clockMs+=10;ui_tick();
        weatherLabel(english?"ความชื้น":"Humidity");weatherLabel("31.0");weatherLabel("กรุงเทพฯ");
        language=selected;clockMs+=10;ui_tick();
        tapTab("หุ้น");capture(("language-stocks-"+suffix+".ppm").c_str());
        assert(findLabel(lv_screen_active(),shapeText(english?"Stocks / Watchlist":"หุ้น / รายการติดตาม").c_str()));
        assert(findLabel(lv_screen_active(),shapeText("หุ้นจำลอง").c_str()));
        tapTab("ตั้งค่า");capture(("language-setup-"+suffix+".ppm").c_str());
        assert(findLabel(lv_screen_active(),shapeText(english?"Desk Duo setup":"ตั้งค่า Desk Duo").c_str()));
        assert(findLabel(lv_screen_active(),"hidden-new-token"));
        language=english?Language::Thai:Language::English;clockMs+=10;ui_tick();
        assert(findLabel(lv_screen_active(),"hidden-new-token"));
        assert(findLabel(lv_screen_active(),shapeText(english?"ตั้งค่า Desk Duo":"Desk Duo setup").c_str()));
        language=selected;clockMs+=10;ui_tick();
        for(const char* tab:{"คู่หู","อากาศ","หุ้น"}) {
            tapTab(tab);assert(!findLabel(lv_screen_active(),"hidden-new-token"));
        }
        const char* markers[]={"Claude","°C",english?"Stocks / Watchlist":"หุ้น / รายการติดตาม",english?"Desk Duo setup":"ตั้งค่า Desk Duo"};
        for(int tab=0;tab<4;++tab) {
            // Exercise top and bottom padding on the same full 60x44 targets
            // after captions change, with a real LVGL input device.
            for(auto point:{std::array<int,2>{tab*60+1,277},std::array<int,2>{tab*60+58,318}}) {
                pointerTapAt(point[0],point[1]);
                assert(findLabel(lv_screen_active(),shapeText(markers[tab]).c_str()));
            }
        }
        tapTab("หุ้น");data=original;
    }
    clockMs+=1000;capture("language-rebuild-start.ppm");
    lv_mem_monitor_t before{},after{};lv_mem_monitor(&before);
    for(int i=0;i<40;++i){language=i%2?Language::Thai:Language::English;clockMs+=20;ui_tick();lv_refr_now(nullptr);}
    language=Language::English;clockMs+=20;ui_tick();lv_refr_now(nullptr);
    lv_mem_monitor(&after);
    std::cout<<"Language rebuild heap before="<<before.free_size<<" after="<<after.free_size<<"\n";
    assert(after.free_size+512>=before.free_size);
    std::cout<<"Thai/English: actual page switches, usage Both/Weekly, Setup reveal, untouched user labels, weather stale/unknown/extremes and font bounds, 40 same-page language rebuilds passed\n";
}
int main(){
    setenv("TZ","Asia/Bangkok",1);tzset();
    lv_init();lv_tick_set_cb([]()->uint32_t{return clockMs;});
    auto* display=lv_display_create(240,320);lv_display_set_color_format(display,LV_COLOR_FORMAT_RGB888);
    lv_display_set_buffers(display,buffer,nullptr,sizeof(buffer),LV_DISPLAY_RENDER_MODE_PARTIAL);
    lv_display_set_flush_cb(display,[](lv_display_t* d,const lv_area_t* a,uint8_t* p){
        for(int y=a->y1;y<=a->y2;++y)for(int x=a->x1;x<=a->x2;++x){
            auto off=(y*240+x)*3;frame[off]=p[2];frame[off+1]=p[1];frame[off+2]=p[0];p+=3;
        }lv_display_flush_ready(d);
    });
    unsigned checks=0;
    for(auto* thai:{&ct_font_thai_12,&ct_font_thai_14}){
        lv_font_t font=thai==&ct_font_thai_12?lv_font_montserrat_12:lv_font_montserrat_14;
        font.fallback=thai;font.line_height=thai->line_height;font.base_line=thai->base_line;
        for(unsigned cp=32;cp<=0xF719;++cp){
            if(!(cp<=126||cp==0xB0||(cp>=0xE01&&cp<=0xE3A)||(cp>=0xE3F&&cp<=0xE5B)||(cp>=0xF700&&cp<=0xF719)))continue;
            lv_font_glyph_dsc_t g{};assert(lv_font_get_glyph_dsc(&font,&g,cp,0));assert(!g.is_placeholder);++checks;
        }
    }
    deskduo::cfg.ssid="SIMULATED";deskduo::cfg.setupPassword="DEMO-password";deskduo::cfg.usageToken="0123456789abcdef0123456789abcdef";
    fakePointer=lv_indev_create();
    lv_indev_set_type(fakePointer,LV_INDEV_TYPE_POINTER);
    lv_indev_set_read_cb(fakePointer,[](lv_indev_t*,lv_indev_data_t* d){d->point.x=pointerX;d->point.y=pointerY;d->state=pointerState;});
    deskduo::ui_init();
    deskduo::ui_tick();
    assert(findLabel(lv_screen_active(),deskduo::shapeText("ไม่ทราบ").c_str()) && "Thai default must translate unknown usage status");
    auto* navScreen=lv_screen_active();
    lv_obj_update_layout(navScreen);
    const char* navNames[]={"คู่หู","อากาศ","หุ้น","ตั้งค่า"};
    std::array<unsigned,4> pointerClicks{};
    for(int i=0;i<4;++i){
        auto shaped=deskduo::shapeText(navNames[i]);
        auto* caption=findLabel(navScreen,shaped.c_str()); assert(caption);
        auto* button=lv_obj_get_parent(caption); assert(button);
        lv_obj_add_event_cb(button,[](lv_event_t* e){++*static_cast<unsigned*>(lv_event_get_user_data(e));},LV_EVENT_CLICKED,&pointerClicks[i]);
        lv_area_t b{},c{};lv_obj_get_coords(button,&b);lv_obj_get_coords(caption,&c);
        assert(b.x1==i*60 && b.x2==i*60+59 && b.y1==276 && b.y2==319);
        const int upperX=i*60+1, lowerX=i*60+58;
        assert(277<c.y1 || 277>c.y2); pointerTapAt(upperX,277);
        assert(pointerClicks[i]==1 && "real LVGL pointer must click upper blank button padding");
        assert(!lv_obj_has_flag(caption,LV_OBJ_FLAG_CLICKABLE)); pointerTapAt(lowerX,318);
        assert(pointerClicks[i]==2 && "real LVGL pointer must click lower blank button padding");
        pointerTapAt(i*60+30,284);
        assert(pointerClicks[i]==3 && "real LVGL pointer must click icon area");
        pointerTapAt((c.x1+c.x2)/2,(c.y1+c.y2)/2);
        assert(pointerClicks[i]==4 && "real LVGL pointer must click caption area");
        const char* pageMarkers[]={"Claude","°C","หุ้น / รายการติดตาม","ตั้งค่า Desk Duo"};
        assert(findLabel(navScreen,deskduo::shapeText(pageMarkers[i]).c_str()) && "pointer clicks must select the requested page");
    }
    pointerTapAt(30,284);
    std::cout<<"LVGL pointer hit actual ui.cpp nav: each 60x44 button accepted top/bottom blank padding, icon, and caption; counts=";
    for(auto n:pointerClicks) std::cout<<n<<",";
    std::cout<<"\n";
    capture("unknown.ppm");
    tapTab("ตั้งค่า");
    assert(findLabel(lv_screen_active(),"0123456789abcdef\n0123456789abcdef"));
    for(const char* tab:{"คู่หู","อากาศ","หุ้น"}) {
        tapTab(tab);
        assert(!findLabel(lv_screen_active(),"0123456789abcdef\n0123456789abcdef"));
    }
    tapTab("คู่หู");
    auto& data=deskduo::data; data.autoRotateSeconds=0;
    deskduo::Usage usage;usage.primary=deskduo::UsageWindow{32,300,1791097200};usage.secondary=deskduo::UsageWindow{48,10080,1791356400};
    data.claude.data=usage;data.claude.unknown=false;data.claude.receivedAtMs=1000;
    usage.primary->usedPercent=58;usage.secondary->usedPercent=21;data.codex.data=usage;data.codex.unknown=false;
    deskduo::Weather w;w.temperature=31;w.humidity=68;w.weatherCode=2;
    w.utcOffsetSeconds=25200;
    w.hourlyCount=4;
    for(int k=0;k<4;++k){w.hourlyTimes[k]=1791097200+k*3600;w.hourlyTemperatures[k]=32+k;w.hourlyRainProbability[k]=35+k;}
    data.weatherLabel="กรุงเทพฯ";data.weather.data=w;data.weather.unknown=false;
    deskduo::Stocks stocks;stocks.count=5;
    for(int k=0;k<5;++k){stocks.items[k].symbol=k==0?"AAPL":k==1?"NVDA":"MSFT";stocks.items[k].price=k==4?1e300:225.40+k;stocks.items[k].changePercent=k%2?-1.24:2.81;stocks.items[k].label="หุ้นจำลอง";stocks.items[k].currency="USD";stocks.items[k].quotedAt=1791097200+k*60;}
    data.stocks.data=stocks;data.stocks.unknown=false;
    deskduo::language=deskduo::Language::English;
    clockMs=4000;capture("duo.ppm");
    assert(findLabel(lv_screen_active(),"7d  48%"));
    assert(findLabel(lv_screen_active(),"Weekly  21%"));
    // Runtime preference changes must update actual LVGL objects immediately,
    // preserve the selected window's reset, and hide all unused Codex objects.
    auto countVisible=[](auto&& self,lv_obj_t* parent,const lv_obj_class_t* cls)->unsigned{
        if(lv_obj_has_flag(parent,LV_OBJ_FLAG_HIDDEN))return 0;
        unsigned n=lv_obj_check_type(parent,cls)?1:0;
        for(uint32_t i=0;i<lv_obj_get_child_count(parent);++i)n+=self(self,lv_obj_get_child(parent,i),cls);
        return n;
    };
    deskduo::mode=deskduo::CodexUsageMode::Weekly;clockMs+=20;capture("duo-weekly.ppm");
    assert(findLabel(lv_screen_active(),"21%"));
    auto* weeklyReset=findLabel(lv_screen_active(),"Weekly reset 07/10 14:00");assert(weeklyReset);
    auto* codexPrimaryBar=findBarBelow(weeklyReset);assert(codexPrimaryBar);
    lv_area_t primaryBarArea;lv_obj_get_coords(codexPrimaryBar,&primaryBarArea);
    assert(countVisible(countVisible,lv_screen_active(),&lv_bar_class)==3);
    assert(lv_obj_has_flag(findLabel(lv_screen_active(),"Weekly  21%"),LV_OBJ_FLAG_HIDDEN));
    data.codex.data->primary=deskduo::UsageWindow{73.25,10080,4102444800};data.codex.data->secondary.reset();
    clockMs+=1000;capture("duo-weekly-primary.ppm");
    assert(findLabel(lv_screen_active(),"73.25%"));assert(findLabel(lv_screen_active(),"Weekly reset 01/01 07:00"));
    data.codex.data->primary->usedPercent=0;clockMs+=1000;deskduo::ui_tick();
    assert(findLabel(lv_screen_active(),"0%"));
    assert(!lv_obj_has_flag(codexPrimaryBar,LV_OBJ_FLAG_HIDDEN) && "eligible zero weekly usage must keep its primary bar visible");
    assert(lv_bar_get_value(codexPrimaryBar)==0);
    data.codex.data->primary->windowMinutes=300;clockMs+=1000;capture("duo-weekly-unavailable.ppm");
    assert(findLabel(lv_screen_active(),"Weekly reset --"));assert(!findLabel(lv_screen_active(),"73.25%"));
    assert(!findLabel(lv_screen_active(),"0%"));
    assert(lv_obj_has_flag(codexPrimaryBar,LV_OBJ_FLAG_HIDDEN) && "no eligible weekly window must not render a zero-valued primary bar");
    assert(countVisible(countVisible,lv_screen_active(),&lv_bar_class)==2);
    lv_area_t hiddenBarArea;lv_obj_get_coords(codexPrimaryBar,&hiddenBarArea);
    assert(hiddenBarArea.x1==primaryBarArea.x1 && hiddenBarArea.y1==primaryBarArea.y1 &&
           hiddenBarArea.x2==primaryBarArea.x2 && hiddenBarArea.y2==primaryBarArea.y2 && "unknown weekly must reserve the primary bar layout space");
    deskduo::mode=deskduo::CodexUsageMode::Both;clockMs+=20;deskduo::ui_tick();
    assert(!lv_obj_has_flag(codexPrimaryBar,LV_OBJ_FLAG_HIDDEN) && "switching to Both must restore the primary bar immediately");
    assert(countVisible(countVisible,lv_screen_active(),&lv_bar_class)==4);
    deskduo::mode=deskduo::CodexUsageMode::Weekly;clockMs+=20;deskduo::ui_tick();
    assert(lv_obj_has_flag(codexPrimaryBar,LV_OBJ_FLAG_HIDDEN));
    data.codex.data->primary->usedPercent=73.25;
    data.codex.data->primary->windowMinutes.reset();clockMs+=1000;deskduo::ui_tick();
    assert(findLabel(lv_screen_active(),"73.25%"));assert(findLabel(lv_screen_active(),"Weekly reset 01/01 07:00"));
    assert(!lv_obj_has_flag(codexPrimaryBar,LV_OBJ_FLAG_HIDDEN) && "eligible weekly data must restore the primary bar after unavailable data");
    deskduo::mode=deskduo::CodexUsageMode::Both;clockMs+=20;deskduo::ui_tick();
    assert(countVisible(countVisible,lv_screen_active(),&lv_bar_class)==4);
    assert(findLabel(lv_screen_active(),"window -- reset 01/01 07:00"));
    data.codex.data->primary=deskduo::UsageWindow{58,90.5,4102444800};
    data.codex.data->secondary=deskduo::UsageWindow{21,2.5,4102531200};clockMs+=1000;deskduo::ui_tick();
    assert(findLabel(lv_screen_active(),"90.5m reset 01/01 07:00"));assert(findLabel(lv_screen_active(),"2.5m  21%"));
    data.codex.data=usage;
    data.claude.data->primary=deskduo::UsageWindow{99.99,90.5,4102444800};
    data.claude.data->secondary=deskduo::UsageWindow{73.25,2.5,4102531200};
    data.claude.stale=true;clockMs+=1000;capture("duo-stale-long-reset.ppm");
    assert(findLabel(lv_screen_active(),"90.5m reset 01/01 07:00"));
    assert(findLabel(lv_screen_active(),"2.5m  73.25%"));
    assert(findLabel(lv_screen_active(),"reset 02/01 07:00"));
    assert(findLabel(lv_screen_active(),"stale 0m"));
    data.claude.data=usage;data.claude.stale=false;
    weatherChecks();
    tapTab("หุ้น");capture("stocks.ppm");
    clockMs=8000;capture("stocks-extremes.ppm");
    data.stocks.data->items[0].label="หุ้นบริษัทภาษาไทยชื่อยาวสำหรับตรวจการแสดงผล";
    data.stocks.stale=true;clockMs=120000;capture("stocks-stale-thai.ppm");
    for(int i=0;i<100;++i){clockMs+=20;deskduo::ui_tick();lv_timer_handler();}
    capture("stocks-thai-scroll.ppm");
    buttonLevel=0;deskduo::ui_tick();clockMs+=100;buttonLevel=1;capture("setup.ppm");
    auto* screen=lv_screen_active();
    assert(findLabel(screen,"0123456789abcdef\n0123456789abcdef"));
    // Token-only rotation does not change the source generation. Open setup
    // must nevertheless replace the revealed credential and reflow its footer.
    deskduo::cfg.usageToken=std::string(128,'A');
    clockMs+=1000;capture("setup-rotated.ppm");
    const char* longToken="AAAAAAAAAAAAAAAA\nAAAAAAAAAAAAAAAA\nAAAAAAAAAAAAAAAA\nAAAAAAAAAAAAAAAA\nAAAAAAAAAAAAAAAA\nAAAAAAAAAAAAAAAA\nAAAAAAAAAAAAAAAA\nAAAAAAAAAAAAAAAA";
    auto* token=findLabel(screen,longToken);
    assert(token && "setup must refresh the active usage token");
    assert(!findLabel(screen,"0123456789abcdef\n0123456789abcdef"));
    auto* footer=findLabel(screen,deskduo::shapeText("BOOT: กลับ / เปิดตั้งค่า").c_str());
    assert(footer && lv_obj_get_y(footer)>=lv_obj_get_y(token)+lv_obj_get_height(token));
    lv_obj_scroll_to_view(footer,LV_ANIM_OFF);capture("setup-long-token-scrolled.ppm");
    lv_area_t scrollArea;lv_obj_get_coords(lv_obj_get_parent(token),&scrollArea);
    assert(scrollArea.y1==24 && scrollArea.y2==255 && "long credentials must scroll inside the content viewport");
    deskduo::cfg.usageToken="fedcba9876543210";
    clockMs+=1000;capture("setup-short-token.ppm");
    token=findLabel(screen,"fedcba9876543210");
    assert(token && !findLabel(screen,longToken));
    assert(lv_obj_get_y(footer)==lv_obj_get_y(token)+lv_obj_get_height(token)+10);
    buttonLevel=0;deskduo::ui_tick();clockMs+=100;buttonLevel=1;capture("setup-closed.ppm");
    assert(!findLabel(screen,"fedcba9876543210"));
    deskduo::cfg.usageToken="hidden-new-token";
    clockMs+=1000;deskduo::ui_tick();
    assert(!findLabel(screen,"hidden-new-token"));
    buttonLevel=0;deskduo::ui_tick();clockMs+=100;buttonLevel=1;capture("setup-reopened.ppm");
    assert(findLabel(screen,"hidden-new-token"));
    data.autoRotateSeconds=5;
    clockMs+=20000;deskduo::ui_tick();
    assert(findLabel(screen,"hidden-new-token") && "automatic rotation must leave an explicitly revealed Setup open");
    tapTab("คู่หู");clockMs+=6000;deskduo::ui_tick();
    assert(findLabel(screen,"Claude") && "manual selection must hold for at least 15 seconds");
    clockMs+=10000;deskduo::ui_tick();
    assert(findLabel(screen,deskduo::shapeText("กรุงเทพฯ").c_str()));
    clockMs+=5000;deskduo::ui_tick();
    assert(findLabel(screen,deskduo::shapeText("หุ้น / รายการติดตาม").c_str()));
    clockMs+=5000;deskduo::ui_tick();
    assert(findLabel(screen,"Claude") && "automatic rotation must skip Setup");
    assert(!findLabel(screen,"hidden-new-token"));
    data.autoRotateSeconds=0;
    lv_mem_monitor_t before{},after{};lv_mem_monitor(&before);
    for(int i=0;i<40;++i) {tapTab("ตั้งค่า");tapTab("คู่หู");}
    lv_mem_monitor(&after);
    assert(after.free_size+512>=before.free_size && "repeated setup reveal/navigation must not leak LVGL heap");
    capture("navigation-cycled.ppm");
    brightnessChecks();
    languageChecks();
    std::cout<<"Setup token rotation, reflow, close/reopen checks passed; ";
    std::cout<<"Navigation/IP, fractional windows, rotation hold/skip checks passed; ";
    std::cout<<"LVGL heap after 40 reveal/close cycles: "<<after.total_size-after.free_size<<" bytes; ";
    std::cout<<"Resolved glyph checks: "<<checks<<"; simulated 240x320 screenshots rendered by actual ui.cpp\n";
}
