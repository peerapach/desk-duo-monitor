#include "lvgl.h"
#include "ui.hpp"
#include "runtime.hpp"
#include "assets.hpp"
#include "fonts/ct_font_thai.h"
#include <cassert>
#include <fstream>
#include <iostream>
#include <cstring>
#include <cstdlib>
#include <ctime>
#include <array>
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
void tapTab(const char* name){
    auto shaped=deskduo::shapeText(name);
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
        const char* pageMarkers[]={"Claude","--°C","หุ้น / Watchlist","ตั้งค่า Pocket Studio"};
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
    for(int k=0;k<4;++k){w.hourlyTimes[k]=1791097200+k*3600;w.hourlyTemperatures[k]=32+k;w.hourlyRainProbability[k]=35+k;}
    data.weatherLabel="กรุงเทพฯ";data.weather.data=w;data.weather.unknown=false;
    deskduo::Stocks stocks;stocks.count=5;
    for(int k=0;k<5;++k){stocks.items[k].symbol=k==0?"AAPL":k==1?"NVDA":"MSFT";stocks.items[k].price=k==4?1e300:225.40+k;stocks.items[k].changePercent=k%2?-1.24:2.81;stocks.items[k].label="หุ้นจำลอง";stocks.items[k].currency="USD";stocks.items[k].quotedAt=1791097200+k*60;}
    data.stocks.data=stocks;data.stocks.unknown=false;
    clockMs=4000;capture("duo.ppm");
    data.claude.data->primary=deskduo::UsageWindow{99.99,90.5,4102444800};
    data.claude.data->secondary=deskduo::UsageWindow{73.25,2.5,4102531200};
    data.claude.stale=true;clockMs+=1000;capture("duo-stale-long-reset.ppm");
    assert(findLabel(lv_screen_active(),"90.5m reset 01/01 07:00"));
    assert(findLabel(lv_screen_active(),"2.5m  73.25%"));
    assert(findLabel(lv_screen_active(),"reset 02/01 07:00"));
    assert(findLabel(lv_screen_active(),"stale 0m"));
    data.claude.data=usage;data.claude.stale=false;
    tapTab("อากาศ");capture("weather.ppm");
    assert(findLabel(lv_screen_active(),"31°C"));
    assert(findLabel(lv_screen_active(),"32°\n35%"));
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
    assert(findLabel(screen,deskduo::shapeText("หุ้น / Watchlist").c_str()));
    clockMs+=5000;deskduo::ui_tick();
    assert(findLabel(screen,"Claude") && "automatic rotation must skip Setup");
    assert(!findLabel(screen,"hidden-new-token"));
    data.autoRotateSeconds=0;
    lv_mem_monitor_t before{},after{};lv_mem_monitor(&before);
    for(int i=0;i<40;++i) {tapTab("ตั้งค่า");tapTab("คู่หู");}
    lv_mem_monitor(&after);
    assert(after.free_size+512>=before.free_size && "repeated setup reveal/navigation must not leak LVGL heap");
    capture("navigation-cycled.ppm");
    std::cout<<"Setup token rotation, reflow, close/reopen checks passed; ";
    std::cout<<"Navigation/IP, fractional windows, rotation hold/skip checks passed; ";
    std::cout<<"LVGL heap after 40 reveal/close cycles: "<<after.total_size-after.free_size<<" bytes; ";
    std::cout<<"Resolved glyph checks: "<<checks<<"; simulated 240x320 screenshots rendered by actual ui.cpp\n";
}
