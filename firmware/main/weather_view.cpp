#include "weather_view.hpp"
#include "assets.hpp"
#include "character_assets.hpp"
#include "fonts/ct_font_thai.h"
#include <algorithm>
#include <array>
#include <cmath>
#include <cstdio>
#include <ctime>

namespace deskduo {
namespace {
constexpr uint32_t black=0x000000, ink=0xF0F1EA, muted=0xA3AAA8;
constexpr uint32_t sun=0xEBA779, water=0xC6E7DC;
enum class Condition { Unknown, Clear, Partial, Cloud, Fog, Drizzle, Rain, Snow, Thunder };
struct View {
    lv_obj_t *location=nullptr, *status=nullptr, *temperature=nullptr, *condition=nullptr;
    lv_obj_t *humidity=nullptr, *icon=nullptr, *unit=nullptr, *neutral=nullptr;
    std::array<lv_obj_t*,3> times{}, temperatures{}, rain{}, icons{};
} view;
lv_font_t thai12, thai14, strong16;
int baselineY(const lv_font_t& font,int baseline) { return baseline-(font.line_height-font.base_line); }
Language language=Language::Thai;
const char* tr(const char* thai,const char* english) { return language==Language::English?english:thai; }

Condition classify(std::optional<int> code) {
    if(!code)return Condition::Unknown;
    // Explicit weather-code groups in the approved brief. Unsupported gaps
    // are intentionally unknown; a numeric range is not a condition group.
    switch(*code) {
        case 0: case 1: return Condition::Clear;
        case 2: return Condition::Partial;
        case 3: return Condition::Cloud;
        case 45: case 48: return Condition::Fog;
        case 51: case 53: case 55: case 56: case 57: return Condition::Drizzle;
        case 61: case 63: case 65: case 66: case 67: case 80: case 81: case 82: return Condition::Rain;
        case 71: case 73: case 75: case 77: case 85: case 86: return Condition::Snow;
        case 95: case 96: case 97: case 99: return Condition::Thunder;
        default: return Condition::Unknown;
    }
}
const char* conditionText(Condition kind) {
    switch(kind) {
        case Condition::Clear:return tr("ท้องฟ้าแจ่มใส","Clear");
        case Condition::Partial:return tr("มีเมฆบางส่วน","Partly cloudy");
        case Condition::Cloud:return tr("เมฆมาก","Cloudy");
        case Condition::Fog:return tr("หมอก","Fog");
        case Condition::Drizzle:return tr("ฝนปรอย","Drizzle");
        case Condition::Rain:return tr("ฝนตก","Rain");
        case Condition::Snow:return tr("หิมะ","Snow");
        case Condition::Thunder:return tr("พายุฝน","Thunderstorm");
        default:return tr("สภาพอากาศ --","Condition --");
    }
}
void setText(lv_obj_t* obj,const std::string& value) {
    const auto shaped=shapeText(value);
    if(shaped!=lv_label_get_text(obj))lv_label_set_text(obj,shaped.c_str());
}
lv_obj_t* label(lv_obj_t* parent,int x,int y,int width,const char* value,
                const lv_font_t* font,uint32_t color=ink,bool centered=false) {
    auto* obj=lv_label_create(parent);
    lv_obj_set_pos(obj,x,y);lv_obj_set_width(obj,width);
    lv_obj_set_style_text_font(obj,font,0);
    lv_obj_set_style_text_color(obj,lv_color_hex(color),0);
    lv_label_set_long_mode(obj,LV_LABEL_LONG_CLIP);
    if(centered)lv_obj_set_style_text_align(obj,LV_TEXT_ALIGN_CENTER,0);
    lv_obj_remove_flag(obj,LV_OBJ_FLAG_CLICKABLE);
    setText(obj,value);return obj;
}
int textWidth(const std::string& value,const lv_font_t* font) {
    lv_point_t size{};lv_text_get_size(&size,value.c_str(),font,0,0,10000,LV_TEXT_FLAG_NONE);
    return size.x;
}
// Bound formatting before printf: huge finite readings must not allocate an
// enormous decimal string or display a clipped scientific-notation fragment.
std::string temperatureText(std::optional<double> value,bool degree=false) {
    if(!value)return "--";
    if(!std::isfinite(*value) || std::abs(*value)>=1000)return tr("เกินช่วง","Out of range");
    char result[16];std::snprintf(result,sizeof(result),"%.1f",*value==0?0:*value);
    return std::string(result)+(degree?"°":"");
}
std::string percentage(std::optional<double> value) {
    if(!value || !std::isfinite(*value) || *value<0 || *value>100)return "--";
    char result[12];std::snprintf(result,sizeof(result),"%.0f%%",*value);return result;
}
std::string timeText(int64_t epoch,std::optional<int32_t> sourceOffset,bool date=false) {
    if(!sourceOffset)return "--";
    // Epochs are normalized UTC by the parser. Shift to the feed's local
    // clock, then format as UTC so the device's timezone is never applied.
    time_t raw=epoch+*sourceOffset;struct tm local{};
    if(!gmtime_r(&raw,&local))return "--";
    char result[24];std::strftime(result,sizeof(result),date?"%d/%m %H:%M":"%H:%M",&local);return result;
}

void illustration(lv_obj_t* obj,std::optional<int> code,std::optional<bool> day,bool mini=false) {
    const auto* image=weather_image(code,day,mini);
    if(image) {lv_image_set_src(obj,image);lv_obj_remove_flag(obj,LV_OBJ_FLAG_HIDDEN);}
    else lv_obj_add_flag(obj,LV_OBJ_FLAG_HIDDEN);
}
void rule(lv_obj_t* parent,int y) {
    auto* obj=lv_obj_create(parent);lv_obj_remove_style_all(obj);lv_obj_set_pos(obj,12,y);lv_obj_set_size(obj,216,1);
    lv_obj_set_style_bg_color(obj,lv_color_hex(0x292F2E),0);lv_obj_set_style_bg_opa(obj,LV_OPA_COVER,0);
    lv_obj_remove_flag(obj,LV_OBJ_FLAG_CLICKABLE);
}
}
void weather_view_reset() { view={}; }
void weather_view_create(lv_obj_t* parent, Language selected) {
    language=selected;
    thai12=lv_font_montserrat_12;thai12.fallback=&ct_font_thai_12;
    thai12.line_height=ct_font_thai_12.line_height;thai12.base_line=ct_font_thai_12.base_line;
    thai14=lv_font_montserrat_14;thai14.fallback=&ct_font_thai_14;
    thai14.line_height=ct_font_thai_14.line_height;thai14.base_line=ct_font_thai_14.base_line;
    strong16=ct_font_ui_16;
    view.location=label(parent,12,baselineY(strong16,17),122,tr("อากาศ","Weather"),&strong16);
    lv_label_set_long_mode(view.location,LV_LABEL_LONG_SCROLL_CIRCULAR);
    view.temperature=label(parent,10,baselineY(ct_font_numeric_48,66),132,"--",&ct_font_numeric_48);
    view.icon=lv_image_create(parent);lv_obj_set_pos(view.icon,134,8);lv_obj_remove_flag(view.icon,LV_OBJ_FLAG_CLICKABLE);
    view.neutral=lv_obj_create(parent);lv_obj_remove_style_all(view.neutral);lv_obj_set_pos(view.neutral,156,30);lv_obj_set_size(view.neutral,52,52);lv_obj_remove_flag(view.neutral,LV_OBJ_FLAG_CLICKABLE);
    label(view.neutral,0,6,52,"?",&lv_font_montserrat_32,muted,true);
    lv_obj_set_style_border_color(view.neutral,lv_color_hex(muted),0);lv_obj_set_style_border_width(view.neutral,2,0);lv_obj_set_style_radius(view.neutral,26,0);lv_obj_set_height(view.neutral,52);
    lv_obj_add_flag(view.neutral,LV_OBJ_FLAG_HIDDEN);
    view.unit=label(parent,124,30,24,"°C",&lv_font_montserrat_14);
    view.condition=label(parent,12,baselineY(strong16,88),132,tr("สภาพอากาศ --","Condition --"),&strong16,ink);
    label(parent,12,baselineY(thai14,117),72,tr("ความชื้น","Humidity"),&thai14,muted);
    view.humidity=label(parent,86,104,54,"--",&lv_font_montserrat_14);
    rule(parent,128);
    label(parent,12,baselineY(thai14,143),216,tr("3 ชั่วโมงถัดไป","Next 3 hours"),&thai14);

    for(int k=0;k<3;++k) {
        const int x=12+72*k;
        view.times[k]=label(parent,x,153,72,"--",&lv_font_montserrat_12,muted,true);
        view.icons[k]=lv_image_create(parent);lv_obj_set_pos(view.icons[k],x+22,169);lv_obj_remove_flag(view.icons[k],LV_OBJ_FLAG_CLICKABLE);
        view.temperatures[k]=label(parent,x,195,72,"--",&lv_font_montserrat_18,ink,true);
        view.rain[k]=label(parent,x,216,72,"--",&lv_font_montserrat_12,water,true);
    }
    for(int x:{84,156}){auto* divider=lv_obj_create(parent);lv_obj_remove_style_all(divider);lv_obj_set_pos(divider,x,154);lv_obj_set_size(divider,1,76);lv_obj_set_style_bg_color(divider,lv_color_hex(0x292F2E),0);lv_obj_set_style_bg_opa(divider,LV_OPA_COVER,0);lv_obj_remove_flag(divider,LV_OBJ_FLAG_CLICKABLE);}
    // Source observation time belongs in the content, above the fixed IP footer.
    view.status=label(parent,12,104,216,tr("ยังไม่มีข้อมูล","No data"),&thai12,muted);
    lv_obj_set_style_text_align(view.status,LV_TEXT_ALIGN_RIGHT,0);
}
void weather_view_update(const Snapshot& snapshot,uint64_t, Language selected) {
    language=selected;if(!view.temperature)return;
    setText(view.location,snapshot.weatherLabel.empty()?tr("อากาศ","Weather"):snapshot.weatherLabel);
    const auto& source=snapshot.weather;
    const Weather* weather=!source.unknown && source.data?&*source.data:nullptr;
    std::string status=tr("ยังไม่มีข้อมูล","No data");
    if(weather) {
        const auto wall=std::time(nullptr);
        const bool clockKnown=wall>=1700000000;
        bool oldObservation=false, showDate=!clockKnown;
        if(weather->updatedAt && clockKnown) {
            // Presentation policy for hourly feeds: two missed observation
            // periods are stale, even when this HTTP receipt just succeeded.
            // Keep the producer's separate transport/receipt stale semantics.
            constexpr double observationStaleSeconds=2*60*60;
            oldObservation=std::difftime(wall,static_cast<time_t>(*weather->updatedAt))>=observationStaleSeconds;
            if(weather->utcOffsetSeconds) {
                time_t observed=*weather->updatedAt+*weather->utcOffsetSeconds;
                time_t current=wall+*weather->utcOffsetSeconds;
                struct tm observedLocal{}, currentLocal{};
                if(gmtime_r(&observed,&observedLocal) && gmtime_r(&current,&currentLocal))
                    showDate=observedLocal.tm_year!=currentLocal.tm_year || observedLocal.tm_yday!=currentLocal.tm_yday;
            }
        }
        status=std::string(source.stale || oldObservation?tr("ข้อมูลเก่า ","Stale "):tr("ข้อมูล ","Data "))+
            (weather->updatedAt?timeText(*weather->updatedAt,weather->utcOffsetSeconds,showDate):"--");
    }
    setText(view.status,status);
    const auto temperature=temperatureText(weather?weather->temperature:std::nullopt);
    const lv_font_t* font=&ct_font_numeric_48;
    if(temperature==tr("เกินช่วง","Out of range"))font=&thai14;
    else if(textWidth(shapeText(temperature),font)>132)font=&lv_font_montserrat_32;
    if(textWidth(shapeText(temperature),font)>132)font=&lv_font_montserrat_14;
    lv_obj_set_style_text_font(view.temperature,font,0);setText(view.temperature,temperature);
    lv_obj_set_x(view.unit,std::min(141,12+textWidth(shapeText(temperature),font)));
    setText(view.condition,conditionText(classify(weather?weather->weatherCode:std::nullopt)));
    setText(view.humidity,percentage(weather?weather->humidity:std::nullopt));
    const bool neutral=weather && weather->weatherCode && (*weather->weatherCode==0 || *weather->weatherCode==1) && !weather->isDay;
    if(neutral)lv_obj_remove_flag(view.neutral,LV_OBJ_FLAG_HIDDEN);else lv_obj_add_flag(view.neutral,LV_OBJ_FLAG_HIDDEN);
    illustration(view.icon,weather?weather->weatherCode:std::nullopt,weather?weather->isDay:std::nullopt);
    if(neutral)lv_obj_add_flag(view.icon,LV_OBJ_FLAG_HIDDEN);
    for(std::size_t k=0;k<3;++k) {
        const bool valid=weather && k<weather->hourlyCount;
        setText(view.times[k],valid && weather->hourlyTimes[k]?timeText(*weather->hourlyTimes[k],weather->utcOffsetSeconds):"--");
        auto temperature=temperatureText(valid?weather->hourlyTemperatures[k]:std::nullopt,true);
        if(textWidth(shapeText(temperature),&lv_font_montserrat_12)>72)temperature="--";
        lv_obj_set_style_text_font(view.temperatures[k],textWidth(shapeText(temperature),&lv_font_montserrat_18)>72?&lv_font_montserrat_12:&lv_font_montserrat_18,0);
        setText(view.temperatures[k],temperature);
        setText(view.rain[k],percentage(valid?weather->hourlyRainProbability[k]:std::nullopt));
        illustration(view.icons[k],valid?weather->hourlyWeatherCodes[k]:std::nullopt,valid?weather->hourlyIsDay[k]:std::nullopt,true);
    }
}
}
