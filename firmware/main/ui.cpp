#include "ui.hpp"
#include "runtime.hpp"
#include "network.hpp"
#include "assets.hpp"
#include "fonts/ct_font_thai.h"
#include "driver/gpio.h"
#include "lvgl.h"
#include <algorithm>
#include <ctime>
#include <cstdio>
namespace deskduo {
namespace {
constexpr uint32_t paper = 0xF3F0E7, ink = 0x292B2D, dim = 0x626B73, line = 0xD4D1C8;
constexpr uint32_t orange = 0xB85F30, robot = 0x425E6D;
lv_font_t text12, text14;
lv_obj_t* screen;
lv_obj_t* clockLabel;
lv_obj_t* content;
lv_obj_t* mascot[2]{};
lv_obj_t* values[16]{};
lv_obj_t* bars[4]{};
int page = 0;
bool setup = false;
uint64_t rotated = 0, refreshed = 0;
lv_obj_t* label(lv_obj_t* parent, int x, int y, int width, const std::string& value, bool large = false) {
    auto* obj = lv_label_create(parent);
    lv_obj_set_pos(obj,x,y); lv_obj_set_width(obj,width);
    lv_obj_set_style_text_font(obj,large ? &lv_font_montserrat_24 : &text12,0);
    lv_obj_set_style_text_color(obj,lv_color_hex(ink),0);
    lv_label_set_long_mode(obj,LV_LABEL_LONG_CLIP);
    auto shaped = shapeText(value); lv_label_set_text(obj,shaped.c_str());
    return obj;
}
void text(lv_obj_t* obj, const std::string& value) {
    auto shaped = shapeText(value);
    // Keep unchanged scrolling labels moving and avoid per-second reallocations.
    if (shaped != lv_label_get_text(obj)) lv_label_set_text(obj,shaped.c_str());
}
void rect(lv_layer_t* layer, int x, int y, int w, int h, uint32_t color) {
    lv_draw_rect_dsc_t d; lv_draw_rect_dsc_init(&d); d.bg_color = lv_color_hex(color); d.bg_opa = LV_OPA_COVER; d.radius = 0;
    lv_area_t area{x,y,x+w-1,y+h-1}; lv_draw_rect(layer,&d,&area);
}
void drawMascot(lv_event_t* event) {
    auto* obj = static_cast<lv_obj_t*>(lv_event_get_target(event));
    const auto provider = reinterpret_cast<uintptr_t>(lv_event_get_user_data(event));
    lv_area_t bounds; lv_obj_get_coords(obj,&bounds);
    int scale = lv_obj_get_width(obj) / 16;
    const char* const* art = provider ? sprite_codex : sprite_claude;
    auto* layer = lv_event_get_layer(event);
    bool blink = (now_ms() + provider * 1700) % 5500 > 5150;
    for (int y=0;y<16;++y) for(int x=0;x<16;) {
        char ch = art[y][x]; int end = x+1;
        while(end<16 && art[y][end]==ch) ++end;
        if(ch!='.') {
            uint32_t color = provider ? (ch=='w' ? 0xFFFFFF : 0x2F7155) : orange;
            if(ch=='e' && !blink) color=paper;
            if(provider && blink && (y==4) && ch=='g' && x>3 && end<13) color=0xFFFFFF;
            rect(layer,bounds.x1+x*scale,bounds.y1+y*scale,(end-x)*scale,scale,color);
        }
        x=end;
    }
}
lv_obj_t* sprite(int provider, int x, int y, int scale) {
    auto* obj = lv_obj_create(content); lv_obj_remove_style_all(obj); lv_obj_set_pos(obj,x,y); lv_obj_set_size(obj,16*scale,16*scale);
    lv_obj_remove_flag(obj,LV_OBJ_FLAG_SCROLLABLE); lv_obj_remove_flag(obj,LV_OBJ_FLAG_CLICKABLE);
    lv_obj_add_event_cb(obj,drawMascot,LV_EVENT_DRAW_MAIN,reinterpret_cast<void*>(static_cast<uintptr_t>(provider)));
    return obj;
}
std::string stamp(std::optional<int64_t> time, bool date=false) {
    if (!time) return "--";
    time_t raw = *time; struct tm local{}; localtime_r(&raw,&local);
    char result[24]; std::strftime(result,sizeof(result),date ? "%d/%m %H:%M" : "%H:%M",&local); return result;
}
template<typename T> std::string age(const SourceSnapshot<T>& source) {
    if (source.unknown) return "unknown";
    auto seconds = (now_ms()-source.receivedAtMs)/1000;
    return std::string(source.stale ? "stale " : "age ") + std::to_string(seconds < 3600 ? seconds/60 : seconds/3600) + (seconds<3600 ? "m" : "h");
}
std::string reading(std::optional<double> number, const char* unit="") { return number ? compactNumber(*number)+unit : "--"; }
std::string windowLabel(const std::optional<UsageWindow>& w) {
    if (!w || !w->windowMinutes) return "window --";
    return *w->windowMinutes < 60 ? compactNumber(*w->windowMinutes) + "m" : compactNumber(*w->windowMinutes / 60) + "h";
}
void switchPage(int next);
void buildPage() {
    if (content) lv_obj_delete(content);
    content=lv_obj_create(screen); lv_obj_remove_style_all(content); lv_obj_set_pos(content,0,54); lv_obj_set_size(content,240,266);
    lv_obj_remove_flag(content,LV_OBJ_FLAG_SCROLLABLE);
    lv_obj_add_flag(content,LV_OBJ_FLAG_GESTURE_BUBBLE);
    std::fill(std::begin(values),std::end(values),nullptr);
    std::fill(std::begin(bars),std::end(bars),nullptr);
    mascot[0]=mascot[1]=nullptr;
    if(setup) {
        auto cfg=runtime_config().config;
        label(content,10,0,220,"ตั้งค่า Desk Duo");
        label(content,10,28,220,ap_name());
        label(content,10,52,220,"AP / admin: " + cfg.setupPassword);
        label(content,10,78,220,"http://192.168.4.1");
        values[0]=label(content,10,103,220,"LAN: "+station_address());
        label(content,10,128,220,"Usage bearer token:");
        // Explicit local reveal only; never in a render snapshot or GET settings.
        std::string tokenLines;
        for(std::size_t i=0;i<cfg.usageToken.size();i+=16) {
            if(i) tokenLines += '\n';
            tokenLines += cfg.usageToken.substr(i,16);
        }
        auto* token=label(content,10,151,220,tokenLines);
        lv_obj_set_style_text_font(token,&lv_font_montserrat_12,0);
        lv_label_set_long_mode(token,LV_LABEL_LONG_WRAP);
        lv_obj_update_layout(token);
        label(content,10,161+lv_obj_get_height(token),220,"BOOT: กลับ / เปิดตั้งค่า");
        lv_obj_add_flag(content,LV_OBJ_FLAG_SCROLLABLE);
        return;
    }
    if(page==0) {
        for(int p=0;p<2;++p) {
            int y=p*111;
            mascot[p]=sprite(p,8,y+30,3);
            label(content,65,y,166,p ? "CODEX" : "CLAUDE");
            values[p*5]=label(content,139,y,94,"unknown");
            for(int k=0;k<2;++k) {
                int index=p*2+k;
                values[p*5+1+k*2]=label(content,65,y+24+k*42,165,"--");
                bars[index]=lv_bar_create(content); lv_obj_set_pos(bars[index],65,y+42+k*42); lv_obj_set_size(bars[index],160,4);
                lv_obj_set_style_bg_color(bars[index],lv_color_hex(line),LV_PART_MAIN);
                lv_obj_set_style_bg_color(bars[index],lv_color_hex(p?robot:orange),LV_PART_INDICATOR);
                values[p*5+2+k*2]=label(content,65,y+48+k*42,165,"reset --");
                // Quota rows contain ASCII only; compact Latin metrics keep both
                // independent resets readable inside each 111px provider panel.
                lv_obj_set_style_text_font(values[p*5+1+k*2], &lv_font_montserrat_12, 0);
                lv_obj_set_style_text_font(values[p*5+2+k*2], &lv_font_montserrat_12, 0);
            }
        }
        label(content,10,239,225,"ปัดเปลี่ยนหน้า  |  BOOT ตั้งค่า");
    } else {
        mascot[0]=sprite(0,9,224,2); mascot[1]=sprite(1,49,224,2);
        values[15]=label(content,97,228,138,"unknown");
        if(page==1) {
            values[0]=label(content,12,0,216,"อากาศ");
            values[1]=label(content,12,35,216,"-- C",true);
            values[2]=label(content,12,73,216,"สภาพอากาศ --");
            values[3]=label(content,12,105,216,"ความชื้น --");
            for(int k=0;k<4;++k) {
                values[4+k]=label(content,10+k*58,143,57,"--");
                values[8+k]=label(content,10+k*58,168,57,"--");
            }
        } else {
            label(content,12,0,216,"หุ้น / Watchlist");
            for(int k=0;k<3;++k) {
                values[k*4]=label(content,12,31+k*61,100,"--");
                values[k*4+1]=label(content,115,31+k*61,117,"--");
                values[k*4+2]=label(content,12,49+k*61,112,"--");
                lv_label_set_long_mode(values[k*4+2],LV_LABEL_LONG_SCROLL_CIRCULAR);
                values[k*4+3]=label(content,143,49+k*61,89,"--");
                values[12+k]=label(content,12,74+k*61,220,"quote --");
                lv_obj_set_style_text_font(values[12+k],&lv_font_montserrat_12,0);
                lv_obj_set_style_text_color(values[12+k],lv_color_hex(dim),0);
            }
        }
    }
}
void switchPage(int next) { setup=false; page=(next+3)%3; rotated=now_ms(); refreshed=0; buildPage(); }
}
void ui_init() {
    // Fallback metrics include high Thai marks and lower descenders.
    text12=lv_font_montserrat_12; text12.fallback=&ct_font_thai_12;
    text12.line_height=ct_font_thai_12.line_height; text12.base_line=ct_font_thai_12.base_line;
    text14=lv_font_montserrat_14; text14.fallback=&ct_font_thai_14;
    text14.line_height=ct_font_thai_14.line_height; text14.base_line=ct_font_thai_14.base_line;
    screen=lv_screen_active(); lv_obj_set_style_bg_color(screen,lv_color_hex(paper),0);
    lv_obj_set_style_text_color(screen,lv_color_hex(ink),0); lv_obj_remove_flag(screen,LV_OBJ_FLAG_SCROLLABLE);
    label(screen,9,-2,120,"DESK DUO"); clockLabel=label(screen,154,-2,80,"--:--");
    const char* tabs[]={"คู่หู","อากาศ","หุ้น"};
    for(int i=0;i<3;++i) {
        auto* button=lv_button_create(screen); lv_obj_set_pos(button,8+i*77,26); lv_obj_set_size(button,70,26);
        lv_obj_set_style_bg_color(button,lv_color_hex(line),0); lv_obj_set_style_border_width(button,0,0);
        lv_obj_set_style_shadow_width(button,0,0); lv_obj_set_style_pad_all(button,0,0);
        label(button,9,-1,65,tabs[i]);
        lv_obj_add_event_cb(button,[](lv_event_t* e){ switchPage(reinterpret_cast<uintptr_t>(lv_event_get_user_data(e))); },LV_EVENT_CLICKED,reinterpret_cast<void*>(static_cast<uintptr_t>(i)));
    }
    lv_obj_add_event_cb(screen,[](lv_event_t*) {
        auto* input=lv_indev_active(); if(!input)return;
        auto direction=lv_indev_get_gesture_dir(input);
        if(direction==LV_DIR_LEFT) switchPage(page+1);
        if(direction==LV_DIR_RIGHT) switchPage(page-1);
        lv_indev_wait_release(input);
    },LV_EVENT_GESTURE,nullptr);
    setup=runtime_config().config.ssid.empty(); rotated=now_ms(); buildPage();
}
void ui_tick() {
    const auto now=now_ms();
    static uint64_t pressStart=0;
    if(!gpio_get_level(GPIO_NUM_0)) { if(!pressStart)pressStart=now; }
    else if(pressStart) {
        if(now-pressStart>=80) { setup=!setup; rotated=now; refreshed=0; buildPage(); }
        pressStart=0;
    }
    auto snapshot=runtime_snapshot();
    if(!setup && snapshot.autoRotateSeconds && now-rotated>=snapshot.autoRotateSeconds*1000ULL) switchPage(page+1);
    for(int p=0;p<2;++p) if(mascot[p]) {
        int baseline=page==0 ? p*111+30 : 224;
        lv_obj_set_y(mascot[p],baseline-(((now+p*750)/750)%2 ? 3 : 0));
        lv_obj_invalidate(mascot[p]);
    }
    if(refreshed && now-refreshed<1000)return;
    refreshed=now;
    const auto wall=std::time(nullptr); text(clockLabel,wall<1700000000 ? "--:--" : stamp(wall));
    if(setup) { text(values[0],"LAN: "+station_address()); return; }
    if(page==0) {
        for(int p=0;p<2;++p) {
            const auto& src=p ? snapshot.codex : snapshot.claude;
            text(values[p*5],age(src));
            for(int k=0;k<2;++k) {
                std::optional<UsageWindow> w;
                if(src.data) w=k ? src.data->secondary : src.data->primary;
                text(values[p*5+1+k*2],windowLabel(w)+"  "+(w ? compactNumber(w->usedPercent)+"%" : "--"));
                lv_bar_set_value(bars[p*2+k],w ? static_cast<int>(w->usedPercent) : 0,LV_ANIM_OFF);
                text(values[p*5+2+k*2],"reset "+stamp(w ? w->resetsAt : std::nullopt,true));
            }
        }
    } else if(page==1) {
        text(values[0],snapshot.weatherLabel.empty() ? "อากาศ" : snapshot.weatherLabel);
        const auto& weather=snapshot.weather.data;
        text(values[1],weather ? reading(weather->temperature," C") : "-- C");
        const char* condition="สภาพอากาศ --";
        if(weather && weather->weatherCode) {
            int code=*weather->weatherCode;
            condition=code==0 ? "ท้องฟ้าแจ่มใส" : code<=3 ? "มีเมฆ" : code<=48 ? "หมอก" : code>=95 ? "พายุฝน" : "ฝน / หิมะ";
        }
        text(values[2],condition);
        text(values[3],"ความชื้น "+(weather ? reading(weather->humidity,"%") : "--"));
        for(int k=0;k<4;++k) {
            text(values[4+k],weather ? stamp(weather->hourlyTimes[k]) : "--");
            text(values[8+k],weather ? reading(weather->hourlyTemperatures[k])+"\n"+reading(weather->hourlyRainProbability[k],"%") : "--\n--");
        }
        text(values[15],age(snapshot.weather));
    } else {
        const auto& stocks=snapshot.stocks.data;
        std::size_t first=stocks && stocks->count>3 ? ((now/6000)%2)*3 : 0;
        for(std::size_t k=0;k<3;++k) {
            if(!stocks || first+k>=stocks->count) { for(int j=0;j<4;++j)text(values[k*4+j],"--"); text(values[12+k],""); continue; }
            const auto& stock=stocks->items[first+k];
            text(values[k*4],stock.symbol);
            text(values[k*4+1],reading(stock.price)+" "+stock.currency);
            text(values[k*4+2],stock.label);
            text(values[12+k],"quote "+stamp(stock.quotedAt,true));
            text(values[k*4+3],reading(stock.changePercent,"%"));
            lv_obj_set_style_text_color(values[k*4+3],lv_color_hex(stock.changePercent && *stock.changePercent<0 ? 0xAD4545 : 0x2F7155),0);
        }
        text(values[15],age(snapshot.stocks));
    }
}
}
