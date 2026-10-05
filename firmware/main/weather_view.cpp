#include "weather_view.hpp"
#include "assets.hpp"
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
struct Point { bool valid=false; int y=50; };
struct View {
    lv_obj_t *location=nullptr, *status=nullptr, *temperature=nullptr, *condition=nullptr;
    lv_obj_t *humidity=nullptr, *icon=nullptr, *plot=nullptr;
    std::array<lv_obj_t*,4> times{}, temperatures{}, rain{};
    std::array<Point,4> points{};
    Condition kind=Condition::Unknown;
} view;
lv_font_t thai12, thai14;
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
std::string timeText(int64_t epoch,std::optional<int32_t> sourceOffset) {
    if(!sourceOffset)return "--";
    // Epochs are normalized UTC by the parser. Shift to the feed's local
    // clock, then format as UTC so the device's timezone is never applied.
    time_t raw=epoch+*sourceOffset;struct tm local{};
    if(!gmtime_r(&raw,&local))return "--";
    char result[12];std::strftime(result,sizeof(result),"%H:%M",&local);return result;
}
void stroke(lv_layer_t* layer,const lv_area_t& bounds,int x1,int y1,int x2,int y2,
            uint32_t color=ink,int width=2) {
    lv_draw_line_dsc_t d;lv_draw_line_dsc_init(&d);
    d.p1={static_cast<lv_value_precise_t>(bounds.x1+x1),static_cast<lv_value_precise_t>(bounds.y1+y1)};
    d.p2={static_cast<lv_value_precise_t>(bounds.x1+x2),static_cast<lv_value_precise_t>(bounds.y1+y2)};
    d.color=lv_color_hex(color);d.width=width;d.round_start=d.round_end=1;lv_draw_line(layer,&d);
}
void circle(lv_layer_t* layer,const lv_area_t& bounds,int x,int y,int radius,uint32_t color,int width=2) {
    lv_draw_arc_dsc_t d;lv_draw_arc_dsc_init(&d);
    d.center={bounds.x1+x,bounds.y1+y};d.radius=radius;d.start_angle=0;d.end_angle=360;
    d.color=lv_color_hex(color);d.width=width;lv_draw_arc(layer,&d);
}
// Original small outline artwork. A black triangle fan masks the sun behind
// the cloud, then rounded line segments trace its softly curved silhouette.
void cloud(lv_layer_t* layer,const lv_area_t& bounds,int dx,int dy) {
    static constexpr int points[][2]={
        {8,46},{5,45},{2,42},{1,39},{1,36},{2,33},{5,31},{8,30},{11,30},
        {12,26},{13,23},{16,20},{19,18},{23,17},{27,17},{31,18},{34,20},
        {36,23},{38,28},{42,28},{46,29},{49,31},{52,34},{53,38},{53,41},
        {51,44},{48,46},{44,47},{8,47}
    };
    for(unsigned i=0;i+1<std::size(points);++i) {
        lv_draw_triangle_dsc_t fill;lv_draw_triangle_dsc_init(&fill);
        fill.bg_color=lv_color_hex(black);fill.bg_opa=LV_OPA_COVER;
        fill.p[0]={static_cast<lv_value_precise_t>(bounds.x1+dx+27),static_cast<lv_value_precise_t>(bounds.y1+dy+37)};
        for(int j=0;j<2;++j)fill.p[j+1]={static_cast<lv_value_precise_t>(bounds.x1+dx+points[i+j][0]),static_cast<lv_value_precise_t>(bounds.y1+dy+points[i+j][1])};
        lv_draw_triangle(layer,&fill);
    }
    for(unsigned i=0;i+1<std::size(points);++i)
        stroke(layer,bounds,dx+points[i][0],dy+points[i][1],dx+points[i+1][0],dy+points[i+1][1]);
}
void sunOutline(lv_layer_t* layer,const lv_area_t& bounds,int x,int y,int radius) {
    circle(layer,bounds,x,y,radius,sun);
    for(int i=0;i<8;++i) {
        double angle=i*3.141592653589793/4;
        stroke(layer,bounds,x+std::lround(std::cos(angle)*(radius+5)),y+std::lround(std::sin(angle)*(radius+5)),
               x+std::lround(std::cos(angle)*(radius+9)),y+std::lround(std::sin(angle)*(radius+9)),sun);
    }
}
void drawIcon(lv_event_t* event) {
    auto* obj=static_cast<lv_obj_t*>(lv_event_get_target(event));
    lv_area_t b;lv_obj_get_coords(obj,&b);auto* layer=lv_event_get_layer(event);
    switch(view.kind) {
        case Condition::Unknown:
            circle(layer,b,32,32,20,muted);stroke(layer,b,26,32,38,32,muted);return;
        case Condition::Clear:sunOutline(layer,b,32,32,14);return;
        case Condition::Partial:sunOutline(layer,b,42,21,11);cloud(layer,b,1,9);return;
        case Condition::Cloud: {
            // Only the visible contour of the rear cloud is drawn; omitting
            // its hidden bottom avoids antialiased seams through the front.
            static constexpr int rear[][2]={{27,20},{30,17},{35,16},{40,17},{43,19},{46,23},{48,27},
                {52,27},{56,28},{60,30},{62,33},{63,37},{63,40},{61,43},{58,45}};
            for(unsigned i=0;i+1<std::size(rear);++i)
                stroke(layer,b,rear[i][0],rear[i][1],rear[i+1][0],rear[i+1][1],muted);
            cloud(layer,b,2,7);return;
        }
        case Condition::Fog:
            cloud(layer,b,4,-6);
            stroke(layer,b,9,48,54,48,water);stroke(layer,b,5,55,47,55,water);stroke(layer,b,16,62,55,62,water);return;
        default:cloud(layer,b,4,-3);break;
    }
    if(view.kind==Condition::Drizzle) {
        for(int x:{17,32,47}) {circle(layer,b,x,53,1,water,1);circle(layer,b,x-2,61,1,water,1);}
    } else if(view.kind==Condition::Rain) {
        for(int x:{17,32,47})stroke(layer,b,x,51,x-4,61,water);
    } else if(view.kind==Condition::Snow) {
        for(int x:{16,32,48}) {
            stroke(layer,b,x,51,x,61,water,1);stroke(layer,b,x-4,53,x+4,59,water,1);stroke(layer,b,x-4,59,x+4,53,water,1);
        }
    } else if(view.kind==Condition::Thunder) {
        stroke(layer,b,34,47,25,55,sun);stroke(layer,b,25,55,33,55,sun);stroke(layer,b,33,55,26,63,sun);
        stroke(layer,b,48,51,44,60,water);
    }
}
void drawPlot(lv_event_t* event) {
    auto* obj=static_cast<lv_obj_t*>(lv_event_get_target(event));
    lv_area_t b;lv_obj_get_coords(obj,&b);auto* layer=lv_event_get_layer(event);
    for(unsigned k=0;k<view.points.size();++k) {
        const auto& point=view.points[k];if(!point.valid)continue;
        if(k && view.points[k-1].valid)
            stroke(layer,b,27+54*(k-1),view.points[k-1].y,27+54*k,point.y,water,1);
        circle(layer,b,27+54*k,point.y,3,water,1);
    }
}
void drawDroplet(lv_event_t* event) {
    auto* obj=static_cast<lv_obj_t*>(lv_event_get_target(event));
    lv_area_t b;lv_obj_get_coords(obj,&b);auto* layer=lv_event_get_layer(event);
    static constexpr int points[][2]={{6,1},{2,7},{1,10},{2,13},{4,15},{7,15},{10,13},{11,10},{10,7},{6,1}};
    for(unsigned i=0;i+1<std::size(points);++i)
        stroke(layer,b,points[i][0],points[i][1],points[i+1][0],points[i+1][1],water,1);
}
lv_obj_t* drawing(lv_obj_t* parent,int x,int y,int width,int height,lv_event_cb_t draw) {
    auto* obj=lv_obj_create(parent);lv_obj_remove_style_all(obj);
    lv_obj_set_pos(obj,x,y);lv_obj_set_size(obj,width,height);
    lv_obj_remove_flag(obj,LV_OBJ_FLAG_CLICKABLE);lv_obj_remove_flag(obj,LV_OBJ_FLAG_SCROLLABLE);
    lv_obj_add_event_cb(obj,draw,LV_EVENT_DRAW_MAIN,nullptr);return obj;
}
}
void weather_view_reset() { view={}; }
void weather_view_create(lv_obj_t* parent, Language selected) {
    language=selected;
    thai12=lv_font_montserrat_12;thai12.fallback=&ct_font_thai_12;
    thai12.line_height=ct_font_thai_12.line_height;thai12.base_line=ct_font_thai_12.base_line;
    thai14=lv_font_montserrat_14;thai14.fallback=&ct_font_thai_14;
    thai14.line_height=ct_font_thai_14.line_height;thai14.base_line=ct_font_thai_14.base_line;
    view.location=label(parent,14,4,122,tr("อากาศ","Weather"),&thai14);
    lv_label_set_long_mode(view.location,LV_LABEL_LONG_SCROLL_CIRCULAR);
    view.status=label(parent,142,9,84,tr("ยังไม่มีข้อมูล","No data"),&thai12,muted);
    lv_obj_set_style_text_align(view.status,LV_TEXT_ALIGN_RIGHT,0);
    view.condition=label(parent,14,28,212,tr("สภาพอากาศ --","Condition --"),&thai12,muted);
    view.temperature=label(parent,12,48,126,"--",&lv_font_montserrat_48);
    label(parent,143,55,24,"°C",&lv_font_montserrat_14);
    view.icon=drawing(parent,162,44,65,65,drawIcon);
    drawing(parent,14,112,12,17,drawDroplet);
    label(parent,33,112,60,tr("ความชื้น","Humidity"),&thai12,muted);
    view.humidity=label(parent,93,113,48,"--",&lv_font_montserrat_14);
    auto* legend=label(parent,159,117,67,tr("โอกาสฝน","Rain %"),&thai12,muted);
    lv_obj_set_style_text_align(legend,LV_TEXT_ALIGN_RIGHT,0);
    view.plot=drawing(parent,12,136,216,90,drawPlot);
    for(int k=0;k<4;++k) {
        view.times[k]=label(parent,12+54*k,136,54,"--",&lv_font_montserrat_12,muted,true);
        view.temperatures[k]=label(parent,12+54*k,164,54,"--",&lv_font_montserrat_14,ink,true);
        view.rain[k]=label(parent,12+54*k,210,54,"--",&lv_font_montserrat_12,water,true);
    }
}
void weather_view_update(const Snapshot& snapshot,uint64_t nowMs, Language selected) {
    language=selected;
    if(!view.temperature)return;
    setText(view.location,snapshot.weatherLabel.empty()?tr("อากาศ","Weather"):snapshot.weatherLabel);
    const auto& source=snapshot.weather;
    const Weather* weather=!source.unknown && source.data?&*source.data:nullptr;
    std::string status=tr("ยังไม่มีข้อมูล","No data");
    if(weather) {
        const uint64_t minutes=(nowMs>=source.receivedAtMs?nowMs-source.receivedAtMs:0)/60000;
        const auto count=minutes<60?minutes:minutes/60;
        const auto duration=(count>999?std::string(">999"):std::to_string(count))+(minutes<60?tr(" น.","m"):tr(" ชม.","h"));
        status=std::string(source.stale?tr("ข้อมูลเก่า ","Stale "):tr("รับแล้ว ","Received "))+duration;
        // Preserve a readable stale marker and receipt age when native Thai
        // metrics make the longer wording wider than its reserved column.
        if(textWidth(shapeText(status),&thai12)>84)
            status=std::string(source.stale?tr("เก่า ","Stale "):tr("รับ ","Rx "))+duration;
    }
    setText(view.status,status);
    const auto value=weather?weather->temperature:std::nullopt;
    const auto temperature=temperatureText(value);
    const auto shaped=shapeText(temperature);
    const lv_font_t* font=&lv_font_montserrat_48;
    if(temperature==tr("เกินช่วง","Out of range"))font=&thai14;
    else if(textWidth(shaped,font)>126)font=&lv_font_montserrat_32;
    lv_obj_set_style_text_font(view.temperature,font,0);setText(view.temperature,temperature);
    view.kind=classify(weather?weather->weatherCode:std::nullopt);
    setText(view.condition,conditionText(view.kind));setText(view.humidity,percentage(weather?weather->humidity:std::nullopt));
    lv_obj_invalidate(view.icon);
    const std::size_t count=weather?std::min<std::size_t>(weather->hourlyCount,4):0;
    double low=0,high=0;bool havePoint=false;
    for(std::size_t k=0;k<4;++k) {
        auto& point=view.points[k];point={};
        point.valid=k<count && weather->hourlyTimes[k] && weather->hourlyTemperatures[k] &&
            std::isfinite(*weather->hourlyTemperatures[k]) && std::abs(*weather->hourlyTemperatures[k])<1000;
        if(point.valid) {
            const double t=*weather->hourlyTemperatures[k];
            if(!havePoint)low=high=t;else {low=std::min(low,t);high=std::max(high,t);}havePoint=true;
        }
    }
    const double padding=std::max(.2,(high-low)*.12);low-=padding;high+=padding;
    for(std::size_t k=0;k<4;++k) {
        auto& point=view.points[k];std::string time="--",temperature="--",rain="--";
        if(point.valid) {
            time=timeText(*weather->hourlyTimes[k],weather->utcOffsetSeconds);temperature=temperatureText(weather->hourlyTemperatures[k],true);
            // Rare wide signed values retain their value at the minimum 12px
            // scale. Ordinary annotations always use the approved 14px font.
            const lv_font_t* font=&lv_font_montserrat_14;
            if(textWidth(temperature,font)>54)font=&lv_font_montserrat_12;
            if(textWidth(temperature,font)>54)temperature="--";
            lv_obj_set_style_text_font(view.temperatures[k],font,0);
            rain=percentage(weather->hourlyRainProbability[k]);
            point.y=std::clamp(static_cast<int>(std::lround(62-((*weather->hourlyTemperatures[k]-low)/(high-low))*24)),38,62);
        } else lv_obj_set_style_text_font(view.temperatures[k],&lv_font_montserrat_14,0);
        setText(view.times[k],time);setText(view.temperatures[k],temperature);setText(view.rain[k],rain);
        lv_obj_set_y(view.temperatures[k],136+point.y-25);
    }
    lv_obj_invalidate(view.plot);
}
}
