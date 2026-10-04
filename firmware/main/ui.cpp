#include "ui.hpp"
#include "runtime.hpp"
#include "network.hpp"
#include "assets.hpp"
#include "character_assets.hpp"
#include "fonts/ct_font_thai.h"
#include "driver/gpio.h"
#include "lvgl.h"
#include <algorithm>
#include <cmath>
#include <ctime>
#include <cstdio>
namespace deskduo {
namespace {
constexpr uint32_t black=0x000000, ink=0xF0F1EA, dim=0xA3AAA8, line=0x292F2E;
constexpr uint32_t copper=0xEBA779, mint=0xC6E7DC;
lv_font_t text12;
lv_obj_t *screen, *clockLabel, *ipLabel, *content;
lv_obj_t* mascot[2]{};
lv_obj_t* values[16]{};
lv_obj_t* bars[4]{};
lv_obj_t* buttons[4]{};
lv_obj_t* captions[4]{};
lv_obj_t* currencies[3]{};
int page=0;
bool setup=false;
uint64_t rotated=0, refreshed=0, holdUntil=0;
lv_obj_t* label(lv_obj_t* parent,int x,int y,int width,const std::string& value,bool large=false) {
    auto* obj=lv_label_create(parent);
    lv_obj_set_pos(obj,x,y); lv_obj_set_width(obj,width);
    lv_obj_set_style_text_font(obj,large?&lv_font_montserrat_32:&text12,0);
    lv_obj_set_style_text_color(obj,lv_color_hex(ink),0);
    lv_label_set_long_mode(obj,LV_LABEL_LONG_CLIP);
    auto shaped=shapeText(value); lv_label_set_text(obj,shaped.c_str());
    return obj;
}
void text(lv_obj_t* obj,const std::string& value) {
    auto shaped=shapeText(value);
    if(shaped!=lv_label_get_text(obj)) lv_label_set_text(obj,shaped.c_str());
}
lv_obj_t* metric(int x,int y,int width,const std::string& value,uint32_t color=ink) {
    auto* obj=label(content,x,y,width,value);
    lv_obj_set_style_text_font(obj,&lv_font_montserrat_12,0);
    lv_obj_set_style_text_color(obj,lv_color_hex(color),0);
    return obj;
}
void rule(lv_obj_t* parent,int x,int y,int width) {
    auto* obj=lv_obj_create(parent); lv_obj_remove_style_all(obj);
    lv_obj_set_pos(obj,x,y); lv_obj_set_size(obj,width,1);
    lv_obj_set_style_bg_color(obj,lv_color_hex(line),0); lv_obj_set_style_bg_opa(obj,LV_OPA_COVER,0);
    lv_obj_remove_flag(obj,LV_OBJ_FLAG_CLICKABLE);
    lv_obj_remove_flag(obj,LV_OBJ_FLAG_SCROLLABLE);
}
lv_obj_t* sprite(unsigned provider,int x,int y,bool mini=false) {
    auto* obj=lv_image_create(content); lv_image_set_src(obj,character_image(provider,mini));
    lv_obj_set_pos(obj,x,y); lv_obj_remove_flag(obj,LV_OBJ_FLAG_CLICKABLE);
    return obj;
}
std::string stamp(std::optional<int64_t> time,bool date=false) {
    if(!time) return "--";
    time_t raw=*time; struct tm local{}; localtime_r(&raw,&local);
    char result[24]; std::strftime(result,sizeof(result),date?"%d/%m %H:%M":"%H:%M",&local); return result;
}
template<typename T> std::string age(const SourceSnapshot<T>& source) {
    if(source.unknown) return "unknown";
    auto seconds=(now_ms()-source.receivedAtMs)/1000;
    return std::string(source.stale?"stale ":"age ")+std::to_string(seconds<3600?seconds/60:seconds/3600)+(seconds<3600?"m":"h");
}
std::string reading(std::optional<double> number,const char* unit="") { return number?compactNumber(*number)+unit:"--"; }
std::string concise(double value) {
    auto result=compactNumber(value);
    if(result.find('e')==std::string::npos && result.find('.')!=std::string::npos) {
        while(result.back()=='0') result.pop_back();
        if(result.back()=='.') result.pop_back();
    }
    return result;
}
std::string windowLabel(const std::optional<UsageWindow>& window) {
    if(!window || !window->windowMinutes) return "window --";
    const double minutes=*window->windowMinutes;
    if(std::fmod(minutes,1440)==0) return concise(minutes/1440)+"d";
    if(std::fmod(minutes,60)==0) return concise(minutes/60)+"h";
    return concise(minutes)+"m";
}
std::string tokenLines(const std::string& token) {
    std::string lines;
    for(std::size_t i=0;i<token.size();i+=16) { if(i)lines+='\n'; lines+=token.substr(i,16); }
    return lines;
}
void navIcon(lv_event_t* event) {
    auto* obj=static_cast<lv_obj_t*>(lv_event_get_target(event));
    const int tab=reinterpret_cast<uintptr_t>(lv_event_get_user_data(event));
    const bool selected=tab==(setup?3:page);
    lv_area_t bounds; lv_obj_get_coords(obj,&bounds);
    auto* layer=lv_event_get_layer(event);
    auto box=[&](int x,int y,int w,int h,int radius,bool fill=false) {
        lv_draw_rect_dsc_t d; lv_draw_rect_dsc_init(&d);
        d.radius=radius; d.bg_color=lv_color_hex(selected?(tab==0?copper:mint):dim);
        d.bg_opa=fill?LV_OPA_COVER:LV_OPA_TRANSP;
        d.border_width=fill?0:2; d.border_color=d.bg_color; d.border_opa=LV_OPA_COVER;
        lv_area_t area{bounds.x1+x,bounds.y1+y,bounds.x1+x+w-1,bounds.y1+y+h-1};
        lv_draw_rect(layer,&d,&area);
    };
    if(tab==0) { box(19,5,14,14,7); box(28,5,14,14,7); }
    if(tab==1) { box(18,11,25,9,4); box(24,5,13,14,7); }
    if(tab==2) { box(20,13,4,7,1,true); box(28,9,4,11,1,true); box(36,5,4,15,1,true); }
    if(tab==3) { box(23,5,15,15,4); box(28,10,5,5,2,true); }
    if(selected) box(14,41,32,2,1,true);
}
void buildPage() {
    if(content) lv_obj_delete(content);
    content=lv_obj_create(screen); lv_obj_remove_style_all(content);
    lv_obj_set_pos(content,0,24); lv_obj_set_size(content,240,232);
    lv_obj_remove_flag(content,LV_OBJ_FLAG_SCROLLABLE); lv_obj_add_flag(content,LV_OBJ_FLAG_GESTURE_BUBBLE);
    std::fill(std::begin(values),std::end(values),nullptr);
    std::fill(std::begin(bars),std::end(bars),nullptr);
    std::fill(std::begin(currencies),std::end(currencies),nullptr);
    mascot[0]=mascot[1]=nullptr;
    for(int i=0;i<4;++i) {
        const bool active=i==(setup?3:page);
        lv_obj_set_style_text_color(captions[i],lv_color_hex(active?(i==0?copper:mint):dim),0);
        lv_obj_invalidate(buttons[i]);
    }
    if(setup) {
        auto cfg=runtime_config().config;
        label(content,10,4,220,"ตั้งค่า Pocket Studio");
        label(content,10,32,220,ap_name());
        label(content,10,56,220,"AP / admin: "+cfg.setupPassword);
        label(content,10,82,220,"http://192.168.4.1");
        values[0]=label(content,10,107,220,"LAN: "+station_address());
        label(content,10,133,220,"Usage bearer token:");
        // Credentials exist only while an explicit local reveal is open.
        values[1]=metric(10,157,220,tokenLines(cfg.usageToken));
        lv_label_set_long_mode(values[1],LV_LABEL_LONG_WRAP); lv_obj_update_layout(values[1]);
        values[2]=label(content,10,167+lv_obj_get_height(values[1]),220,"BOOT: กลับ / เปิดตั้งค่า");
        lv_obj_add_flag(content,LV_OBJ_FLAG_SCROLLABLE);
        lv_obj_set_scroll_dir(content,LV_DIR_VER);
        lv_obj_set_scrollbar_mode(content,LV_SCROLLBAR_MODE_AUTO);
        return;
    }
    if(page==0) {
        for(int p=0;p<2;++p) {
            const int y=p*116;
            mascot[p]=sprite(p,8,y+25);
            auto* name=metric(82,y+4,78,p?"Codex":"Claude");
            lv_obj_set_style_text_font(name,&lv_font_montserrat_14,0);
            values[p*6]=metric(151,y+5,81,"unknown",dim);
            lv_obj_set_style_text_align(values[p*6],LV_TEXT_ALIGN_RIGHT,0);
            values[p*6+1]=label(content,82,y+19,150,"--",true);
            values[p*6+2]=metric(82,y+55,150,"window -- reset --",dim);
            lv_label_set_long_mode(values[p*6+2],LV_LABEL_LONG_SCROLL_CIRCULAR);
            values[p*6+3]=metric(82,y+76,150,"window --  --");
            values[p*6+4]=metric(82,y+94,150,"reset --",dim);
            for(int k=0;k<2;++k) {
                auto* bar=bars[p*2+k]=lv_bar_create(content);
                lv_obj_remove_style_all(bar);
                lv_obj_set_pos(bar,82,y+(k?109:69)); lv_obj_set_size(bar,148,3);
                lv_obj_set_style_bg_color(bar,lv_color_hex(line),LV_PART_MAIN);
                lv_obj_set_style_bg_opa(bar,LV_OPA_COVER,LV_PART_MAIN);
                lv_obj_set_style_bg_color(bar,lv_color_hex(p?mint:copper),LV_PART_INDICATOR);
                lv_obj_set_style_bg_opa(bar,LV_OPA_COVER,LV_PART_INDICATOR);
                lv_obj_set_style_radius(bar,2,LV_PART_MAIN);
                lv_obj_set_style_radius(bar,2,LV_PART_INDICATOR);
            }
        }
        rule(content,10,115,220);
    } else {
        mascot[0]=sprite(0,9,197,true); mascot[1]=sprite(1,48,197,true);
        values[15]=metric(92,213,138,"unknown",dim);
        lv_obj_set_style_text_align(values[15],LV_TEXT_ALIGN_RIGHT,0);
        if(page==1) {
            values[0]=label(content,12,5,216,"อากาศ");
            lv_label_set_long_mode(values[0],LV_LABEL_LONG_SCROLL_CIRCULAR);
            values[1]=label(content,12,34,216,"--°C",true);
            values[2]=label(content,12,75,216,"สภาพอากาศ --");
            values[3]=label(content,12,104,216,"ความชื้น --");
            rule(content,10,135,220);
            for(int k=0;k<4;++k) {
                values[4+k]=metric(10+k*58,143,57,"--",dim);
                values[8+k]=metric(10+k*58,163,57,"--\n--");
                lv_label_set_long_mode(values[8+k],LV_LABEL_LONG_SCROLL_CIRCULAR);
            }
        } else {
            label(content,12,4,216,"หุ้น / Watchlist");
            for(int k=0;k<3;++k) {
                int y=33+k*54;
                values[k*4]=metric(12,y+3,69,"--");
                lv_label_set_long_mode(values[k*4],LV_LABEL_LONG_SCROLL_CIRCULAR);
                values[k*4+1]=metric(84,y,107,"--");
                lv_obj_set_style_text_font(values[k*4+1],&lv_font_montserrat_18,0);
                lv_obj_set_style_text_align(values[k*4+1],LV_TEXT_ALIGN_RIGHT,0);
                lv_label_set_long_mode(values[k*4+1],LV_LABEL_LONG_SCROLL_CIRCULAR);
                currencies[k]=metric(193,y+4,37,"",dim);
                lv_obj_set_style_text_align(currencies[k],LV_TEXT_ALIGN_RIGHT,0);
                lv_label_set_long_mode(currencies[k],LV_LABEL_LONG_SCROLL_CIRCULAR);
                values[k*4+2]=label(content,12,y+20,132,"--");
                lv_label_set_long_mode(values[k*4+2],LV_LABEL_LONG_SCROLL_CIRCULAR);
                values[k*4+3]=metric(147,y+23,83,"--");
                lv_obj_set_style_text_align(values[k*4+3],LV_TEXT_ALIGN_RIGHT,0);
                values[12+k]=metric(12,y+38,218,"quote --",dim);
                if(k<2)rule(content,12,y+51,218);
            }
        }
    }
}
void switchPage(int next,bool manual=true,bool reveal=false) {
    setup=reveal;
    if(!setup)page=(next+3)%3;
    rotated=now_ms(); if(manual)holdUntil=rotated+15000;
    refreshed=0; buildPage();
}
}
void ui_init() {
    text12=lv_font_montserrat_12; text12.fallback=&ct_font_thai_12;
    text12.line_height=ct_font_thai_12.line_height; text12.base_line=ct_font_thai_12.base_line;
    screen=lv_screen_active(); lv_obj_set_style_bg_color(screen,lv_color_hex(black),0);
    lv_obj_set_style_text_color(screen,lv_color_hex(ink),0); lv_obj_remove_flag(screen,LV_OBJ_FLAG_SCROLLABLE);
    auto* title=label(screen,10,1,160,"pocket studio"); lv_obj_set_style_text_font(title,&lv_font_montserrat_14,0);
    clockLabel=label(screen,185,3,45,"--:--"); lv_obj_set_style_text_font(clockLabel,&lv_font_montserrat_12,0);
    lv_obj_set_style_text_align(clockLabel,LV_TEXT_ALIGN_RIGHT,0); rule(screen,10,23,220);
    ipLabel=label(screen,0,259,240,station_address()); lv_obj_set_style_text_font(ipLabel,&lv_font_montserrat_12,0);
    lv_obj_set_style_text_color(ipLabel,lv_color_hex(dim),0); lv_obj_set_style_text_align(ipLabel,LV_TEXT_ALIGN_CENTER,0);
    lv_obj_remove_flag(ipLabel,LV_OBJ_FLAG_CLICKABLE); rule(screen,10,256,220); rule(screen,10,275,220);
    const char* tabs[]={"คู่หู","อากาศ","หุ้น","ตั้งค่า"};
    for(int i=0;i<4;++i) {
        auto* button=buttons[i]=lv_button_create(screen); lv_obj_remove_style_all(button);
        lv_obj_set_pos(button,i*60,276); lv_obj_set_size(button,60,44);
        lv_obj_set_style_bg_color(button,lv_color_hex(black),0); lv_obj_set_style_bg_opa(button,LV_OPA_COVER,0);
        auto* caption=captions[i]=label(button,0,19,60,tabs[i]); lv_obj_set_style_text_align(caption,LV_TEXT_ALIGN_CENTER,0);
        lv_obj_add_event_cb(button,navIcon,LV_EVENT_DRAW_MAIN,reinterpret_cast<void*>(static_cast<uintptr_t>(i)));
        lv_obj_add_event_cb(button,[](lv_event_t* e){
            int tab=reinterpret_cast<uintptr_t>(lv_event_get_user_data(e));
            switchPage(tab,true,tab==3);
        },LV_EVENT_CLICKED,reinterpret_cast<void*>(static_cast<uintptr_t>(i)));
    }
    lv_obj_add_event_cb(screen,[](lv_event_t*) {
        auto* input=lv_indev_active(); if(!input)return;
        auto direction=lv_indev_get_gesture_dir(input);
        if(direction==LV_DIR_LEFT)switchPage(page+1);
        if(direction==LV_DIR_RIGHT)switchPage(page-1);
        lv_indev_wait_release(input);
    },LV_EVENT_GESTURE,nullptr);
    setup=runtime_config().config.ssid.empty(); rotated=now_ms(); buildPage();
}
void ui_tick() {
    const auto now=now_ms(); static uint64_t pressStart=0;
    if(!gpio_get_level(GPIO_NUM_0)) { if(!pressStart)pressStart=now; }
    else if(pressStart) { if(now-pressStart>=80)switchPage(page,true,!setup); pressStart=0; }
    auto snapshot=runtime_snapshot();
    if(!setup && snapshot.autoRotateSeconds && now>=holdUntil && now-rotated>=snapshot.autoRotateSeconds*1000ULL)switchPage(page+1,false);
    // A one-pixel, four-second breath is independent of usage/session activity.
    for(int p=0;p<2;++p)if(mascot[p]) {
        int baseline=page==0?p*116+25:197;
        lv_obj_set_y(mascot[p],baseline-static_cast<int>(std::lround(std::sin((now+p*600)*0.0015707963))));
    }
    if(refreshed && now-refreshed<1000)return;
    refreshed=now;
    const auto wall=std::time(nullptr); text(clockLabel,wall<1700000000?"--:--":stamp(wall));
    text(ipLabel,station_address());
    if(setup) {
        text(values[0],"LAN: "+station_address());
        text(values[1],tokenLines(runtime_config().config.usageToken)); lv_obj_update_layout(values[1]);
        lv_obj_set_y(values[2],167+lv_obj_get_height(values[1])); return;
    }
    if(page==0) {
        for(int p=0;p<2;++p) {
            const auto& src=p?snapshot.codex:snapshot.claude;
            text(values[p*6],age(src));
            std::optional<UsageWindow> primary,secondary;
            if(src.data) { primary=src.data->primary; secondary=src.data->secondary; }
            text(values[p*6+1],primary?concise(primary->usedPercent)+"%":"--");
            text(values[p*6+2],windowLabel(primary)+" reset "+stamp(primary?primary->resetsAt:std::nullopt,true));
            text(values[p*6+3],windowLabel(secondary)+"  "+(secondary?concise(secondary->usedPercent)+"%":"--"));
            text(values[p*6+4],"reset "+stamp(secondary?secondary->resetsAt:std::nullopt,true));
            for(int k=0;k<2;++k) { const auto& w=k?secondary:primary; lv_bar_set_value(bars[p*2+k],w?static_cast<int>(w->usedPercent):0,LV_ANIM_OFF); }
        }
    } else if(page==1) {
        text(values[0],snapshot.weatherLabel.empty()?"อากาศ":snapshot.weatherLabel);
        const auto& weather=snapshot.weather.data;
        text(values[1],weather && weather->temperature?concise(*weather->temperature)+"°C":"--°C");
        const char* condition="สภาพอากาศ --";
        if(weather && weather->weatherCode) {
            int code=*weather->weatherCode;
            condition=code==0?"ท้องฟ้าแจ่มใส":code<=3?"มีเมฆ":code<=48?"หมอก":code>=95?"พายุฝน":"ฝน / หิมะ";
        }
        text(values[2],condition); text(values[3],"ความชื้น "+(weather && weather->humidity?concise(*weather->humidity)+"%":"--"));
        for(int k=0;k<4;++k) {
            text(values[4+k],weather?stamp(weather->hourlyTimes[k]):"--");
            std::string temperature=weather && weather->hourlyTemperatures[k]?concise(*weather->hourlyTemperatures[k])+"°":"--";
            std::string rain=weather && weather->hourlyRainProbability[k]?concise(*weather->hourlyRainProbability[k])+"%":"--";
            text(values[8+k],temperature+"\n"+rain);
        }
        text(values[15],age(snapshot.weather));
    } else {
        const auto& stocks=snapshot.stocks.data;
        std::size_t first=stocks && stocks->count>3?((now/6000)%2)*3:0;
        for(std::size_t k=0;k<3;++k) {
            if(!stocks || first+k>=stocks->count) { for(int j=0;j<4;++j)text(values[k*4+j],"--"); text(values[12+k],""); text(currencies[k],""); continue; }
            const auto& stock=stocks->items[first+k];
            text(values[k*4],stock.symbol); text(values[k*4+1],reading(stock.price)); text(currencies[k],stock.currency);
            text(values[k*4+2],stock.label); text(values[12+k],"quote "+stamp(stock.quotedAt,true));
            text(values[k*4+3],reading(stock.changePercent,"%"));
            lv_obj_set_style_text_color(values[k*4+3],lv_color_hex(stock.changePercent && *stock.changePercent<0?copper:mint),0);
        }
        text(values[15],age(snapshot.stocks));
    }
}
}
