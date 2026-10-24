#include "ui.hpp"
#include "runtime.hpp"
#include "backlight.hpp"
#include "brightness_policy.hpp"
#include "ota_policy.hpp"
#include "network.hpp"
#include "assets.hpp"
#include "character_assets.hpp"
#include "fonts/ct_font_thai.h"
#include "weather_view.hpp"
#include "stock_view.hpp"
#include "driver/gpio.h"
#include "lvgl.h"
#include <algorithm>
#include <cmath>
#include <ctime>
#include <cstdio>
namespace deskduo {
namespace {
constexpr uint32_t black=0x000000, ink=0xF0F1EA, dim=0xA3AAA8, line=0x292F2E;
constexpr uint32_t copper=0xFB8452, mint=0x82CEBD;
constexpr int duoStride=112,duoMascotY=49,duoPrimaryBaseline=24;
lv_font_t text12, text14, textStrong, metricLatin12, metricThai12;
lv_obj_t *screen, *clockLabel, *ipLabel, *content, *titleLabel;
lv_obj_t* mascot[2]{};
lv_obj_t* values[16]{};
lv_obj_t* bars[4]{};
lv_obj_t* resetLabels[4]{};
lv_obj_t* buttons[4]{};
lv_obj_t* captions[4]{};
lv_obj_t* navImages[4]{};
int page=0;
bool setup=false;
CodexUsageMode displayMode=CodexUsageMode::Both;
Language language=Language::Thai;
lv_obj_t *brightnessSlider=nullptr, *brightnessLabel=nullptr, *brightnessStatus=nullptr, *scheduleSwitch=nullptr;
bool brightnessPreview=false, brightnessFailed=false;
int appliedBrightness=-1;
enum class BrightnessStatus { Saved, Preview, SaveFailed, ApplyFailed };
BrightnessStatus brightnessState=BrightnessStatus::Saved;
// Place a shared TH/EN role by its typographic baseline, independent of reserved mark space.
int baselineY(const lv_font_t& font,int baseline) { return baseline-(font.line_height-font.base_line); }
const char* tr(const char* thai,const char* english) { return language==Language::English?english:thai; }
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
    const auto* font=lv_obj_get_style_text_font(obj,0);
    if(font==&metricLatin12 || font==&metricThai12) {
        // Keep the original metric baseline. Thai fallback needs a taller
        // line box, whose extra ascent must not push glyphs onto usage bars.
        bool thai=false;
        for(std::size_t i=0;i+1<value.size();++i)
            if(static_cast<uint8_t>(value[i])==0xE0 &&
               (static_cast<uint8_t>(value[i+1])==0xB8 || static_cast<uint8_t>(value[i+1])==0xB9))thai=true;
        const auto* next=thai?&metricThai12:&metricLatin12;
        if(next!=font) {
            const int oldAscent=font->line_height-font->base_line;
            const int newAscent=next->line_height-next->base_line;
            lv_obj_set_y(obj,lv_obj_get_style_y(obj,0)+oldAscent-newAscent);
            lv_obj_set_style_text_font(obj,next,0);
        }
    }
    auto shaped=shapeText(value);
    if(shaped!=lv_label_get_text(obj)) lv_label_set_text(obj,shaped.c_str());
}
void fitHeader() {
    const lv_font_t* font=&ct_font_ui_18;
    auto bounds=[&](const lv_font_t* candidate){
        int top=100,bottom=-100;uint32_t offset=0;const auto* value=lv_label_get_text(titleLabel);
        while(value[offset]) {
            unsigned cp=static_cast<unsigned char>(value[offset++]);
            unsigned count=cp<128?0:cp<224?1:cp<240?2:3;
            if(count)cp&=(1U<<(6-count))-1;
            while(count-- && value[offset])cp=(cp<<6)|(static_cast<unsigned char>(value[offset++])&63);
            lv_font_glyph_dsc_t glyph{};
            if(!lv_font_get_glyph_dsc(candidate,&glyph,cp,0) || !glyph.box_h)continue;
            const auto* descriptor=static_cast<const lv_font_fmt_txt_dsc_t*>(candidate->dsc);
            const auto& bitmapGlyph=descriptor->glyph_dsc[glyph.gid.index];
            const auto* pixels=descriptor->glyph_bitmap+bitmapGlyph.bitmap_index;
            for(unsigned row=0;row<glyph.box_h;++row)for(unsigned x=0;x<glyph.box_w;++x){
                const unsigned bit=(row*glyph.box_w+x)*descriptor->bpp;
                const auto opacity=(pixels[bit/8]>>(8-descriptor->bpp-bit%8))&((1U<<descriptor->bpp)-1);
                if(!opacity)continue;
                const int y=-glyph.ofs_y-glyph.box_h+static_cast<int>(row);top=std::min(top,y);bottom=std::max(bottom,y);
            }
        }
        return std::pair<int,int>{top,bottom};
    };
    auto inkBounds=bounds(font);
    if(inkBounds.second-inkBounds.first+1>29){font=&ct_font_ui_16;inkBounds=bounds(font);}
    const int preferred=language==Language::Thai?22:18;
    const int baseline=std::clamp(preferred,-inkBounds.first,28-inkBounds.second);
    lv_obj_set_style_text_font(titleLabel,font,0);lv_obj_set_y(titleLabel,baselineY(*font,baseline));
}
lv_obj_t* metric(int x,int y,int width,const std::string& value,uint32_t color=ink) {
    auto* obj=label(content,x,y,width,value);
    lv_obj_set_style_text_font(obj,&metricLatin12,0);
    text(obj,value);
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
    if(source.unknown) return tr("ไม่ทราบ","unknown");
    auto seconds=(now_ms()-source.receivedAtMs)/1000;
    return std::string(source.stale?tr("เก่า ","stale "):tr("รับ ","age "))+std::to_string(seconds<3600?seconds/60:seconds/3600)+(seconds<3600?tr("น.","m"):tr("ชม.","h"));
}
std::string concise(double value) {
    auto result=compactNumber(value);
    if(result.find('e')==std::string::npos && result.find('.')!=std::string::npos) {
        while(result.back()=='0') result.pop_back();
        if(result.back()=='.') result.pop_back();
    }
    return result;
}
std::string windowLabel(const std::optional<UsageWindow>& window,bool codex=false) {
    if(!window || !window->windowMinutes) return tr("ช่วง --","window --");
    const double minutes=*window->windowMinutes;
    if(codex && minutes==10080) return tr("สัปดาห์","Weekly");
    if(std::fmod(minutes,1440)==0) return concise(minutes/1440)+tr("วัน","d");
    if(std::fmod(minutes,60)==0) return concise(minutes/60)+tr("ชม.","h");
    return concise(minutes)+tr("น.","m");
}
void usagePercent(lv_obj_t* obj,const std::string& value,const std::string& window,int rowY) {
    const auto shaped=shapeText(window);
    lv_point_t numberSize{},windowSize{};
    lv_text_get_size(&numberSize,value.c_str(),&ct_font_numeric_24,0,0,10000,LV_TEXT_FLAG_NONE);
    lv_text_get_size(&windowSize,shaped.c_str(),&text14,0,0,10000,LV_TEXT_FLAG_NONE);
    // Long/fractional windows share the same row. Preserve the exact value and
    // baseline, using the smaller full font only when the two columns need it.
    const auto* font=value.find_first_not_of("0123456789.%+-")==std::string::npos &&
        numberSize.x<=92 && windowSize.x+numberSize.x+8<=136?&ct_font_numeric_24:&ct_font_ui_16;
    lv_obj_set_style_text_font(obj,font,0);
    lv_obj_set_y(obj,rowY+baselineY(*font,duoPrimaryBaseline));
    text(obj,value);
}
std::string tokenLines(const std::string& token) {
    std::string lines;
    for(std::size_t i=0;i<token.size();i+=16) { if(i)lines+='\n'; lines+=token.substr(i,16); }
    return lines;
}
bool applyBrightness(uint8_t percent) {
    if(appliedBrightness==percent)return true;
    if(display_set_brightness(percent)!=ESP_OK)return false;
    appliedBrightness=percent;return true;
}
void brightnessText() {
    if(!brightnessSlider)return;
    auto preview=runtime_display_settings();
    preview.brightnessPercent=static_cast<uint8_t>(lv_slider_get_value(brightnessSlider));
    text(brightnessLabel,tr("ความสว่างฐาน","Base brightness"));
    text(values[7],std::to_string(preview.brightnessPercent)+"%");
    const char* status=brightnessState==BrightnessStatus::Preview?tr("ปล่อยเพื่อบันทึก","Release to save"):
        brightnessState==BrightnessStatus::SaveFailed?tr("บันทึกไม่สำเร็จ","Could not save"):
        brightnessState==BrightnessStatus::ApplyFailed?tr("ปรับแสงไม่สำเร็จ","Brightness failed"):"";
    text(brightnessStatus,status);
}
void cancelBrightness() {
    const auto committed=runtime_display_settings();
    if(brightnessPreview && !applyBrightness(brightness_plan(committed,std::time(nullptr)).percent))brightnessState=BrightnessStatus::ApplyFailed;
    else if(brightnessPreview && !brightnessFailed)brightnessState=BrightnessStatus::Saved;
    brightnessPreview=false;brightnessFailed=false;
    if(brightnessSlider) { lv_slider_set_value(brightnessSlider,committed.brightnessPercent,LV_ANIM_OFF);brightnessText(); }
}
void brightnessEvent(lv_event_t* event) {
    const auto code=lv_event_get_code(event);
    if(code==LV_EVENT_INDEV_RESET) {if(brightnessPreview)cancelBrightness();return;}
    if(code!=LV_EVENT_PRESSED && code!=LV_EVENT_VALUE_CHANGED && code!=LV_EVENT_PRESS_LOST && code!=LV_EVENT_RELEASED)return;
    // LVGL's slider class can re-enable scroll chaining on release/resize.
    lv_obj_remove_flag(brightnessSlider,static_cast<lv_obj_flag_t>(LV_OBJ_FLAG_SCROLL_CHAIN|LV_OBJ_FLAG_GESTURE_BUBBLE));
    if(code==LV_EVENT_PRESSED || code==LV_EVENT_VALUE_CHANGED) {
        if(!brightnessPreview){brightnessPreview=true;brightnessFailed=false;}
        if(!brightnessFailed) {
            auto selected=runtime_display_settings();
            selected.brightnessPercent=static_cast<uint8_t>(lv_slider_get_value(brightnessSlider));
            if(applyBrightness(brightness_plan(selected,std::time(nullptr)).percent))brightnessState=BrightnessStatus::Preview;
            else {brightnessFailed=true;brightnessState=BrightnessStatus::ApplyFailed;}
        }
        brightnessText();
    } else if(code==LV_EVENT_PRESS_LOST)cancelBrightness();
    else if(code==LV_EVENT_RELEASED && brightnessPreview) {
        const auto selected=static_cast<uint8_t>(lv_slider_get_value(brightnessSlider));
        if(!brightnessFailed) {
            MaintenanceLease lease(Maintenance::Settings);
            if(!lease)brightnessState=BrightnessStatus::SaveFailed;
            else {
                // Clock and committed schedule can change during a held drag.
                // Apply the latest derived selection under the save's lease.
                auto candidate=runtime_display_settings();candidate.brightnessPercent=selected;
                if(!applyBrightness(brightness_plan(candidate,std::time(nullptr)).percent))brightnessState=BrightnessStatus::ApplyFailed;
                else if(runtime_display_patch("{\"brightnessPercent\":"+std::to_string(selected)+"}")!=204)brightnessState=BrightnessStatus::SaveFailed;
                else brightnessState=BrightnessStatus::Saved;
            }
        }
        const auto committed=runtime_display_settings();
        if(!applyBrightness(brightness_plan(committed,std::time(nullptr)).percent))brightnessState=BrightnessStatus::ApplyFailed;
        brightnessPreview=false;brightnessFailed=false;
        lv_slider_set_value(brightnessSlider,committed.brightnessPercent,LV_ANIM_OFF);brightnessText();
    }
}
void navIcon(lv_event_t* event) {
    auto* obj=static_cast<lv_obj_t*>(lv_event_get_target(event));
    const int tab=reinterpret_cast<uintptr_t>(lv_event_get_user_data(event));
    const bool selected=tab==(setup?3:page);
    lv_area_t bounds; lv_obj_get_coords(obj,&bounds);
    auto* layer=lv_event_get_layer(event);
    if(selected){lv_draw_rect_dsc_t d;lv_draw_rect_dsc_init(&d);d.radius=2;d.bg_color=lv_color_hex(mint);d.bg_opa=LV_OPA_COVER;lv_area_t a{bounds.x1+24,bounds.y1+1,bounds.x1+35,bounds.y1+3};lv_draw_rect(layer,&d,&a);}

}
void buildPage() {
    cancelBrightness();
    brightnessSlider=brightnessLabel=brightnessStatus=scheduleSwitch=nullptr;
    weather_view_reset();
    stock_view_reset();
    text(titleLabel,setup?tr("ตั้งค่า","Setup"):page==0?tr("คู่หู","Duo"):page==1?tr("อากาศ","Weather"):tr("หุ้น","Stocks"));
    fitHeader();
    if(content) lv_obj_delete(content);
    content=lv_obj_create(screen); lv_obj_remove_style_all(content);
    lv_obj_set_pos(content,0,24); lv_obj_set_size(content,240,232);
    lv_obj_remove_flag(content,LV_OBJ_FLAG_SCROLLABLE); lv_obj_add_flag(content,LV_OBJ_FLAG_GESTURE_BUBBLE);
    std::fill(std::begin(values),std::end(values),nullptr);
    std::fill(std::begin(bars),std::end(bars),nullptr);
    std::fill(std::begin(resetLabels),std::end(resetLabels),nullptr);
    mascot[0]=mascot[1]=nullptr;
    for(int i=0;i<4;++i) {
        const bool active=i==(setup?3:page);
        const char* thai[]={"คู่หู","อากาศ","หุ้น","ตั้งค่า"};
        const char* english[]={"Duo","Weather","Stocks","Setup"};
        text(captions[i],tr(thai[i],english[i]));
        lv_obj_set_style_text_color(captions[i],lv_color_hex(active?ink:dim),0);
        lv_image_set_src(navImages[i],navigation_image(i,active));
        lv_obj_invalidate(buttons[i]);
    }
    if(setup) {
        auto cfg=runtime_config().config;

        values[7]=label(content,186,baselineY(text14,66),44,"");lv_obj_set_style_text_font(values[7],&text14,0);lv_obj_set_style_text_align(values[7],LV_TEXT_ALIGN_RIGHT,0);
        brightnessLabel=label(content,12,baselineY(text14,66),174,"");lv_obj_set_style_text_font(brightnessLabel,&text14,0);
        brightnessSlider=lv_slider_create(content);lv_obj_remove_style_all(brightnessSlider);
        lv_obj_set_pos(brightnessSlider,12,84);lv_obj_set_size(brightnessSlider,216,6);
        lv_obj_set_ext_click_area(brightnessSlider,20);
        lv_slider_set_range(brightnessSlider,5,100);
        lv_slider_set_value(brightnessSlider,runtime_display_settings().brightnessPercent,LV_ANIM_OFF);
        for(auto part:{LV_PART_MAIN,LV_PART_INDICATOR,LV_PART_KNOB}) {
            lv_obj_set_style_bg_color(brightnessSlider,lv_color_hex(part==LV_PART_MAIN?line:part==LV_PART_KNOB?ink:mint),part);
            lv_obj_set_style_bg_opa(brightnessSlider,LV_OPA_COVER,part);
            lv_obj_set_style_radius(brightnessSlider,8,part);
        }
        lv_obj_set_style_pad_all(brightnessSlider,5,LV_PART_KNOB);
        lv_obj_remove_flag(brightnessSlider,static_cast<lv_obj_flag_t>(LV_OBJ_FLAG_SCROLL_CHAIN|LV_OBJ_FLAG_GESTURE_BUBBLE|LV_OBJ_FLAG_SCROLL_ON_FOCUS));
        lv_obj_add_flag(brightnessSlider,LV_OBJ_FLAG_PRESS_LOCK);
        lv_obj_add_event_cb(brightnessSlider,brightnessEvent,LV_EVENT_ALL,nullptr);
        brightnessStatus=label(content,12,94,216,"");brightnessText();
        values[4]=label(content,12,baselineY(textStrong,21),110,tr("ภาษา","Language"));lv_obj_set_style_text_font(values[4],&textStrong,0);
        for(int selected=0;selected<2;++selected) {
            auto* button=lv_button_create(content);lv_obj_remove_style_all(button);
            lv_obj_set_pos(button,126+selected*52,0);lv_obj_set_size(button,52,44);
            auto* segment=lv_obj_create(button);lv_obj_remove_style_all(segment);lv_obj_set_pos(segment,0,6);lv_obj_set_size(segment,52,26);
            lv_obj_set_style_border_color(segment,lv_color_hex((language==Language::English)==bool(selected)?mint:line),0);
            lv_obj_set_style_border_width(segment,1,0);lv_obj_set_style_radius(segment,6,0);lv_obj_remove_flag(segment,LV_OBJ_FLAG_CLICKABLE);
            auto* caption=label(button,0,6,52,selected?"EN":"TH");lv_obj_set_style_text_align(caption,LV_TEXT_ALIGN_CENTER,0);
            lv_obj_remove_flag(caption,LV_OBJ_FLAG_CLICKABLE);
            lv_obj_add_event_cb(button,[](lv_event_t* event) {
                MaintenanceLease lease(Maintenance::Settings);if(!lease)return;
                const bool english=reinterpret_cast<uintptr_t>(lv_event_get_user_data(event));
                const auto status=runtime_display_patch(english?"{\"language\":\"en\"}":"{\"language\":\"th\"}");
                if(status!=204)text(values[4],tr("บันทึกไม่สำเร็จ","Could not save"));
                refreshed=0;
            },LV_EVENT_CLICKED,reinterpret_cast<void*>(static_cast<uintptr_t>(selected)));
        }
        values[3]=label(content,12,baselineY(text14,122),180,tr("ปรับแสงตามเวลา","Scheduled brightness"));lv_obj_set_style_text_font(values[3],&text14,0);
        auto* scheduleTarget=lv_button_create(content);lv_obj_remove_style_all(scheduleTarget);lv_obj_set_pos(scheduleTarget,186,94);lv_obj_set_size(scheduleTarget,44,44);
        scheduleSwitch=lv_switch_create(scheduleTarget);lv_obj_remove_style_all(scheduleSwitch);lv_obj_set_pos(scheduleSwitch,6,10);lv_obj_set_size(scheduleSwitch,32,18);lv_obj_remove_flag(scheduleSwitch,LV_OBJ_FLAG_CLICKABLE);
        lv_obj_set_style_bg_opa(scheduleSwitch,LV_OPA_COVER,LV_PART_MAIN);lv_obj_set_style_bg_color(scheduleSwitch,lv_color_hex(line),LV_PART_MAIN);lv_obj_set_style_radius(scheduleSwitch,9,LV_PART_MAIN);
        lv_obj_set_style_bg_opa(scheduleSwitch,LV_OPA_COVER,LV_PART_INDICATOR|LV_STATE_CHECKED);lv_obj_set_style_bg_color(scheduleSwitch,lv_color_hex(mint),LV_PART_INDICATOR|LV_STATE_CHECKED);lv_obj_set_style_radius(scheduleSwitch,9,LV_PART_INDICATOR);
        lv_obj_set_style_bg_opa(scheduleSwitch,LV_OPA_COVER,LV_PART_KNOB);lv_obj_set_style_bg_color(scheduleSwitch,lv_color_hex(ink),LV_PART_KNOB);lv_obj_set_style_radius(scheduleSwitch,LV_RADIUS_CIRCLE,LV_PART_KNOB);lv_obj_set_style_pad_all(scheduleSwitch,-3,LV_PART_KNOB);
        lv_obj_add_event_cb(scheduleTarget,[](lv_event_t*){
            MaintenanceLease lease(Maintenance::Settings);
            if(!lease){brightnessState=BrightnessStatus::SaveFailed;brightnessText();return;}
            const bool next=!runtime_display_settings().brightnessAuto;
            const int result=runtime_display_patch(next?"{\"brightnessAuto\":true}":"{\"brightnessAuto\":false}");
            brightnessState=result==204?BrightnessStatus::Saved:BrightnessStatus::SaveFailed;
            brightnessText();refreshed=0;
        },LV_EVENT_CLICKED,nullptr);
        values[5]=label(content,12,baselineY(text14,149),216,"");lv_obj_set_style_text_font(values[5],&text14,0);
        values[6]=label(content,12,baselineY(text14,171),216,"");lv_obj_set_style_text_font(values[6],&text14,0);
        rule(content,12,181,216);
        values[0]=label(content,40,baselineY(text14,200),190,tr("เชื่อมต่อแล้ว","Connected"));lv_obj_set_style_text_font(values[0],&text14,0);lv_label_set_long_mode(values[0],LV_LABEL_LONG_SCROLL_CIRCULAR);
        values[9]=label(content,40,197,190,cfg.ssid);lv_label_set_long_mode(values[9],LV_LABEL_LONG_SCROLL_CIRCULAR);
        auto* wifi=lv_obj_create(content);lv_obj_remove_style_all(wifi);lv_obj_set_pos(wifi,12,188);lv_obj_set_size(wifi,22,18);lv_obj_remove_flag(wifi,LV_OBJ_FLAG_CLICKABLE);
        lv_obj_add_event_cb(wifi,[](lv_event_t* e){lv_area_t b;lv_obj_get_coords(static_cast<lv_obj_t*>(lv_event_get_target(e)),&b);auto* layer=lv_event_get_layer(e);for(int k=0;k<3;++k){lv_draw_arc_dsc_t d;lv_draw_arc_dsc_init(&d);d.color=lv_color_hex(mint);d.width=2;d.center={b.x1+11,b.y1+17};d.radius=5+k*6;d.start_angle=225;d.end_angle=315;lv_draw_arc(layer,&d);}},LV_EVENT_DRAW_MAIN,nullptr);
        label(content,12,210,216,tr("ตั้งค่าเพิ่มเติมผ่านเว็บ","More settings on the web"));
        label(content,10,258,220,ap_name());
        label(content,10,284,220,std::string(tr("AP / ผู้ดูแล: ","AP / admin: "))+cfg.setupPassword);
        label(content,10,310,220,"http://192.168.4.1");
        label(content,10,336,220,tr("โทเค็นส่งข้อมูล:","Usage bearer token:"));
        // Credentials remain available in the explicit local Setup reveal.
        values[1]=metric(10,360,220,tokenLines(cfg.usageToken));
        lv_label_set_long_mode(values[1],LV_LABEL_LONG_WRAP); lv_obj_update_layout(values[1]);
        values[2]=label(content,10,370+lv_obj_get_height(values[1]),220,tr("BOOT: กลับ / เปิดตั้งค่า","BOOT: return / setup"));
        lv_obj_add_flag(content,LV_OBJ_FLAG_SCROLLABLE);
        lv_obj_set_scroll_dir(content,LV_DIR_VER);
        lv_obj_set_scrollbar_mode(content,LV_SCROLLBAR_MODE_AUTO);
        return;
    }
    if(page==0) {
        for(int p=0;p<2;++p) {
            const int y=p*duoStride;
            mascot[p]=sprite(p,10,y+duoMascotY);
            auto* name=label(content,10,y+baselineY(ct_font_ui_16,27),74,p?"Codex":"Claude");
            lv_obj_set_style_text_font(name,&ct_font_ui_16,0);
            values[p*6]=metric(10,y+baselineY(metricLatin12,42),74,"",dim);lv_obj_add_flag(values[p*6],LV_OBJ_FLAG_HIDDEN);
            values[p*6+1]=label(content,138,y+baselineY(ct_font_numeric_24,duoPrimaryBaseline),92,"--");
            lv_obj_set_style_text_font(values[p*6+1],&ct_font_numeric_24,0);
            lv_obj_set_style_text_align(values[p*6+1],LV_TEXT_ALIGN_RIGHT,0);
            values[p*6+2]=label(content,94,y+baselineY(text14,duoPrimaryBaseline),84,"--");lv_obj_set_style_text_font(values[p*6+2],&text14,0);
            values[p*6+3]=label(content,94,y+baselineY(text14,78),84,"--");lv_obj_set_style_text_font(values[p*6+3],&text14,0);
            values[p*6+4]=label(content,138,y+baselineY(text14,78),92,"--");lv_obj_set_style_text_font(values[p*6+4],&text14,0);
            lv_obj_set_style_text_align(values[p*6+4],LV_TEXT_ALIGN_RIGHT,0);
            for(int k=0;k<2;++k) {
                auto* bar=bars[p*2+k]=lv_bar_create(content);lv_obj_remove_style_all(bar);
                lv_obj_set_pos(bar,94,y+(k?85:31));lv_obj_set_size(bar,136,8);
                lv_obj_set_style_bg_color(bar,lv_color_hex(0x25292D),LV_PART_MAIN);lv_obj_set_style_bg_opa(bar,LV_OPA_COVER,LV_PART_MAIN);
                lv_obj_set_style_bg_color(bar,lv_color_hex(p?(k?0x8DCFC2:0x82CEBD):(k?0xE89C78:0xFB8452)),LV_PART_INDICATOR);lv_obj_set_style_bg_opa(bar,LV_OPA_COVER,LV_PART_INDICATOR);
                lv_obj_set_style_radius(bar,4,LV_PART_MAIN);lv_obj_set_style_radius(bar,4,LV_PART_INDICATOR);
                resetLabels[p*2+k]=metric(94,y+baselineY(metricLatin12,k?113:59),136,"",dim);
            }
        }
        rule(content,10,118,220);
    } else if(page==1) {
        weather_view_create(content,language);
    } else {
        stock_view_create(content,language,[](StockChartRange range){
            MaintenanceLease lease(Maintenance::Settings);
            if(!lease)return false;
            const int result=runtime_patch(range==StockChartRange::Day?
                "{\"stocks\":{\"chartRange\":\"1d\"}}":"{\"stocks\":{\"chartRange\":\"5d\"}}");
            refreshed=0;
            return result==204;
        });
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
    language=runtime_language();
    text12=lv_font_montserrat_12; text12.fallback=&ct_font_thai_12;
    text12.line_height=ct_font_thai_12.line_height; text12.base_line=ct_font_thai_12.base_line;
    text14=lv_font_montserrat_14;text14.fallback=&ct_font_thai_14;text14.line_height=ct_font_thai_14.line_height;text14.base_line=ct_font_thai_14.base_line;
    textStrong=ct_font_ui_16;
    metricLatin12=lv_font_montserrat_12;
    metricThai12=text12;
    screen=lv_screen_active(); lv_obj_set_style_bg_color(screen,lv_color_hex(black),0);
    lv_obj_set_style_text_color(screen,lv_color_hex(ink),0); lv_obj_remove_flag(screen,LV_OBJ_FLAG_SCROLLABLE);
    titleLabel=label(screen,10,1,160,"Desk Duo");
    clockLabel=label(screen,185,3,45,"--:--"); lv_obj_set_style_text_font(clockLabel,&lv_font_montserrat_12,0);
    lv_obj_set_style_text_align(clockLabel,LV_TEXT_ALIGN_RIGHT,0);
    ipLabel=label(screen,0,259,240,station_address()); lv_obj_set_style_text_font(ipLabel,&lv_font_montserrat_12,0);
    lv_obj_set_style_text_color(ipLabel,lv_color_hex(dim),0); lv_obj_set_style_text_align(ipLabel,LV_TEXT_ALIGN_CENTER,0);
    lv_obj_remove_flag(ipLabel,LV_OBJ_FLAG_CLICKABLE); rule(screen,10,256,220); rule(screen,10,275,220);
    const char* tabs[]={"คู่หู","อากาศ","หุ้น","ตั้งค่า"};
    for(int i=0;i<4;++i) {
        auto* button=buttons[i]=lv_button_create(screen); lv_obj_remove_style_all(button);
        lv_obj_set_pos(button,i*60,276); lv_obj_set_size(button,60,44);
        lv_obj_set_style_bg_color(button,lv_color_hex(black),0); lv_obj_set_style_bg_opa(button,LV_OPA_COVER,0);
        navImages[i]=lv_image_create(button);lv_image_set_src(navImages[i],navigation_image(i,false));lv_obj_set_pos(navImages[i],18,6);lv_obj_remove_flag(navImages[i],LV_OBJ_FLAG_CLICKABLE);
        auto* caption=captions[i]=label(button,0,24,60,tabs[i]); lv_obj_set_style_text_align(caption,LV_TEXT_ALIGN_CENTER,0);
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
    const auto preferences=runtime_display_settings();
    const auto nextMode=preferences.mode;
    if(preferences.language!=language) {
        language=preferences.language;refreshed=0;buildPage();
    }
    if(nextMode!=displayMode) { displayMode=nextMode; refreshed=0; }
    if(!brightnessPreview) {
        if(!applyBrightness(brightness_plan(preferences,std::time(nullptr)).percent))brightnessState=BrightnessStatus::ApplyFailed;
        if(brightnessSlider) {lv_slider_set_value(brightnessSlider,preferences.brightnessPercent,LV_ANIM_OFF);brightnessText();}
    }
    const auto now=now_ms(); static uint64_t pressStart=0;
    if(!gpio_get_level(GPIO_NUM_0)) { if(!pressStart)pressStart=now; }
    else if(pressStart) { if(now-pressStart>=80)switchPage(page,true,!setup); pressStart=0; }
    auto snapshot=runtime_snapshot();
    if(!setup && snapshot.autoRotateSeconds && now>=holdUntil && now-rotated>=snapshot.autoRotateSeconds*1000ULL)switchPage(page+1,false);
    // A one-pixel, four-second breath is independent of usage/session activity.
    for(int p=0;p<2;++p)if(mascot[p]) {
        int baseline=page==0?p*duoStride+duoMascotY:197;
        lv_obj_set_y(mascot[p],baseline-static_cast<int>(std::lround(std::sin((now+p*600)*0.0015707963))));
    }
    if(refreshed && now-refreshed<1000)return;
    refreshed=now;
    const auto wall=std::time(nullptr); text(clockLabel,wall<1700000000?"--:--":stamp(wall));
    text(ipLabel,station_address());
    if(setup) {
        text(values[0],tr("เชื่อมต่อแล้ว","Connected"));text(values[9],runtime_config().config.ssid);
        text(values[1],tokenLines(runtime_config().config.usageToken)); lv_obj_update_layout(values[1]);
        lv_obj_set_y(values[2],370+lv_obj_get_height(values[1]));
        const auto settings=runtime_display_settings();
        if(settings.brightnessAuto)lv_obj_add_state(scheduleSwitch,LV_STATE_CHECKED);else lv_obj_remove_state(scheduleSwitch,LV_STATE_CHECKED);
        text(values[5],std::string(tr("กลางวัน        ","Day             "))+brightness_time_string(settings.brightnessDayStart));
        text(values[6],std::string(tr("กลางคืน       ","Night           "))+brightness_time_string(settings.brightnessNightStart));
        return;
    }
    if(page==0) {
        for(int p=0;p<2;++p) {
            const auto& src=p?snapshot.codex:snapshot.claude;
            const bool weekly=p==1 && displayMode==CodexUsageMode::Weekly;
            auto selected=weekly && src.data?codexWeeklyWindow(*src.data):std::nullopt;
            text(values[p*6],weekly && !selected?tr("ไม่ทราบ","unknown"):age(src));
            if(src.stale && (now<src.receivedAtMs || now-src.receivedAtMs>=3600000ULL*10000))text(values[p*6],tr("เก่า","Stale"));
            if(src.unknown || src.stale || (weekly && !selected))lv_obj_remove_flag(values[p*6],LV_OBJ_FLAG_HIDDEN);
            else lv_obj_add_flag(values[p*6],LV_OBJ_FLAG_HIDDEN);
            if(weekly && !selected)lv_obj_add_flag(bars[p*2],LV_OBJ_FLAG_HIDDEN);
            else lv_obj_remove_flag(bars[p*2],LV_OBJ_FLAG_HIDDEN);
            for(auto* unused:{values[p*6+3],values[p*6+4],bars[p*2+1],resetLabels[p*2+1]}) {
                if(weekly)lv_obj_add_flag(unused,LV_OBJ_FLAG_HIDDEN);
                else lv_obj_remove_flag(unused,LV_OBJ_FLAG_HIDDEN);
            }
            std::optional<UsageWindow> primary,secondary;
            if(src.data) { primary=src.data->primary; secondary=src.data->secondary; }
            if(weekly) primary=selected;
            const auto primaryWindow=weekly?std::string(tr("สัปดาห์","Weekly")):windowLabel(primary,p==1);
            usagePercent(values[p*6+1],primary?concise(primary->usedPercent)+"%":"--",primaryWindow,p*duoStride);
            text(values[p*6+2],primaryWindow);
            text(values[p*6+3],windowLabel(secondary,true));
            text(values[p*6+4],secondary?concise(secondary->usedPercent)+"%":"--");

            for(int k=0;k<2;++k) {
                const auto& w=k?secondary:primary;
                lv_bar_set_value(bars[p*2+k],w?static_cast<int>(w->usedPercent):0,LV_ANIM_OFF);
                text(resetLabels[p*2+k],std::string(tr("เริ่มใหม่ ","Reset "))+stamp(w?w->resetsAt:std::nullopt,true));
            }
        }
    } else if(page==1) {
        weather_view_update(snapshot,now,language);
    } else {
        stock_view_update(snapshot,now,language);
    }
}
}
