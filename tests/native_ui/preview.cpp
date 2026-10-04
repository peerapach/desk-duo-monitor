#include "lvgl.h"
#include "ui.hpp"
#include "runtime.hpp"
#include "assets.hpp"
#include "fonts/ct_font_thai.h"
#include <cassert>
#include <fstream>
#include <iostream>
#include <cstring>
int buttonLevel=1;
uint64_t clockMs=2000;
namespace deskduo {
Config cfg;
Snapshot data;
uint64_t now_ms(){return clockMs;}
RequestConfig runtime_config(){return {cfg,0};}
Snapshot runtime_snapshot(){return data;}
std::string ap_name(){return "DeskDuo-DEMO01";}
std::string station_address(){return "192.0.2.10";}
}
uint8_t frame[240*320*3]{};
uint8_t buffer[240*24*3];
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
int main(){
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
    deskduo::ui_init();capture("unknown.ppm");
    auto& data=deskduo::data; data.autoRotateSeconds=0;
    deskduo::Usage usage;usage.primary=deskduo::UsageWindow{32,300,1791097200};usage.secondary=deskduo::UsageWindow{48,10080,1791356400};
    data.claude.data=usage;data.claude.unknown=false;data.claude.receivedAtMs=1000;
    usage.primary->usedPercent=58;usage.secondary->usedPercent=21;data.codex.data=usage;data.codex.unknown=false;
    deskduo::Weather w;w.temperature=31;w.humidity=68;w.weatherCode=2;
    for(int k=0;k<4;++k){w.hourlyTimes[k]=1791097200+k*3600;w.hourlyTemperatures[k]=32+k;w.hourlyRainProbability[k]=35+k;}
    data.weatherLabel="กรุงเทพฯ";data.weather.data=w;data.weather.unknown=false;
    deskduo::Stocks stocks;stocks.count=5;
    for(int k=0;k<5;++k){stocks.items[k].symbol=k==0?"AAPL":k==1?"NVDA":"MSFT";stocks.items[k].price=k==4?1e300:225.40+k;stocks.items[k].changePercent=k%2?-1.24:2.81;stocks.items[k].label="หุ้นจำลอง";stocks.items[k].currency="USD";stocks.items[k].quotedAt=1791097200+k*60;}
    data.stocks.data=stocks;data.stocks.unknown=false;
    clockMs=4000;capture("duo.ppm");
    lv_obj_send_event(lv_obj_get_child(lv_screen_active(),3),LV_EVENT_CLICKED,nullptr);capture("weather.ppm");
    lv_obj_send_event(lv_obj_get_child(lv_screen_active(),4),LV_EVENT_CLICKED,nullptr);capture("stocks.ppm");
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
    std::cout<<"Setup token rotation, reflow, close/reopen checks passed; ";
    std::cout<<"Resolved glyph checks: "<<checks<<"; 12 simulated 240x320 screenshots rendered by actual ui.cpp\n";
}
