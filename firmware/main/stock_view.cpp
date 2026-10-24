#include "stock_view.hpp"
#include "assets.hpp"
#include "fonts/ct_font_thai.h"
#include <algorithm>
#include <cmath>
#include <cstdio>
namespace deskduo {
namespace {
constexpr uint32_t ink=0xF0F1EA, muted=0xA3AAA8, mint=0x8AE4CC, copper=0xF5874B;
struct ChartPoint { bool valid=false; int x=0,y=0; };
struct Row {
    lv_obj_t *symbol=nullptr,*detail=nullptr,*price=nullptr,*change=nullptr,*plot=nullptr;
    std::array<ChartPoint,32> points{};
    std::size_t count=0;
    uint32_t color=mint;
};
struct View {
    std::array<Row,4> rows{};
    std::array<lv_obj_t*,2> periods{},captions{};
    lv_obj_t *status=nullptr,*pagination=nullptr;
    StockRangeSave save=nullptr;
    StockChartRange range=StockChartRange::Day;
    bool failed=false;
} view;
Language language=Language::Thai;
lv_font_t thai12;
const char* tr(const char* th,const char* en){return language==Language::English?en:th;}
void text(lv_obj_t* obj,const std::string& value){auto shaped=shapeText(value);if(shaped!=lv_label_get_text(obj))lv_label_set_text(obj,shaped.c_str());}
lv_obj_t* label(lv_obj_t* parent,int x,int y,int w,const char* value,uint32_t color=ink){
    auto* obj=lv_label_create(parent);lv_obj_set_pos(obj,x,y);lv_obj_set_width(obj,w);
    lv_obj_set_style_text_font(obj,&thai12,0);lv_obj_set_style_text_color(obj,lv_color_hex(color),0);
    lv_label_set_long_mode(obj,LV_LABEL_LONG_CLIP);lv_obj_remove_flag(obj,LV_OBJ_FLAG_CLICKABLE);text(obj,value);return obj;
}
void selection(){for(int k=0;k<2;++k){const bool active=k==static_cast<int>(view.range);lv_obj_set_style_text_color(view.captions[k],lv_color_hex(active?ink:muted),0);auto* segment=lv_obj_get_child(view.periods[k],0);lv_obj_set_style_border_color(segment,lv_color_hex(active?mint:0x39413F),0);lv_obj_set_style_bg_opa(segment,active?LV_OPA_20:LV_OPA_TRANSP,0);}}
void choose(lv_event_t* event){
    auto next=static_cast<StockChartRange>(reinterpret_cast<uintptr_t>(lv_event_get_user_data(event)));
    if(next==view.range)return;
    view.failed=!view.save || !view.save(next);
    if(!view.failed){view.range=next;selection();for(auto& row:view.rows){row.count=0;lv_obj_invalidate(row.plot);}}
    text(view.status,view.failed?tr("บันทึกไม่สำเร็จ","Could not save"):tr("กำลังโหลด","Loading"));
}
void draw(lv_event_t* event){
    auto* row=static_cast<Row*>(lv_event_get_user_data(event));
    lv_area_t bounds{};lv_obj_get_coords(static_cast<lv_obj_t*>(lv_event_get_target(event)),&bounds);
    auto* layer=lv_event_get_layer(event);
    for(std::size_t k=1;k<row->count;++k){
        const auto& a=row->points[k-1];const auto& b=row->points[k];if(!a.valid || !b.valid)continue;
        lv_draw_line_dsc_t d;lv_draw_line_dsc_init(&d);d.color=lv_color_hex(row->color);d.width=2;d.round_start=d.round_end=1;
        d.p1={static_cast<lv_value_precise_t>(bounds.x1+a.x),static_cast<lv_value_precise_t>(bounds.y1+a.y)};
        d.p2={static_cast<lv_value_precise_t>(bounds.x1+b.x),static_cast<lv_value_precise_t>(bounds.y1+b.y)};lv_draw_line(layer,&d);
    }
}
std::string number(std::optional<double> v,bool change=false){
    if(!v || !std::isfinite(*v) || std::abs(*v)>=1e9)return "--";
    char out[32];std::snprintf(out,sizeof(out),change?"%+.2f%%":"%.2f",*v==0?0:*v);return out;
}
void chart(Row& row,const Stock* stock){
    row.count=0;
    if(!stock || !stock->history || stock->chartRange!=view.range){lv_obj_invalidate(row.plot);return;}
    row.count=std::min<std::size_t>(stock->historyCount,row.points.size());
    double low=0,high=0;bool any=false;
    for(std::size_t k=0;k<row.count;++k){
        const auto v=(*stock->history)[k].close;auto& p=row.points[k];p.valid=v && std::isfinite(*v) && *v>=0;
        if(p.valid){if(!any)low=high=*v;else {low=std::min(low,*v);high=std::max(high,*v);}any=true;}
    }
    // Each retained sample gets its own slot, so null samples break the line.
    for(std::size_t k=0;k<row.count;++k){auto& p=row.points[k];p.x=row.count>1?static_cast<int>(k*54/(row.count-1)):27;
        if(p.valid)p.y=high==low?16:static_cast<int>(std::lround(28-((*stock->history)[k].close.value()-low)/(high-low)*24));}
    lv_obj_invalidate(row.plot);
}
}
void stock_view_reset(){view={};}
void stock_view_create(lv_obj_t* parent,Language selected,StockRangeSave save){
    language=selected;view.save=save;
    thai12=lv_font_montserrat_12;thai12.fallback=&ct_font_thai_12;thai12.line_height=ct_font_thai_12.line_height;thai12.base_line=ct_font_thai_12.base_line;
    for(int k=0;k<2;++k){auto* b=view.periods[k]=lv_button_create(parent);lv_obj_remove_style_all(b);lv_obj_set_pos(b,10+k*110,0);lv_obj_set_size(b,110,44);
        auto* segment=lv_obj_create(b);lv_obj_remove_style_all(segment);lv_obj_set_pos(segment,0,6);lv_obj_set_size(segment,110,26);lv_obj_set_style_border_width(segment,1,0);lv_obj_set_style_radius(segment,7,0);lv_obj_set_style_bg_color(segment,lv_color_hex(mint),0);lv_obj_remove_flag(segment,LV_OBJ_FLAG_CLICKABLE);
        view.captions[k]=label(b,0,6,110,k?tr("5 วัน","5 days"):tr("วันนี้","Today"));lv_obj_set_style_text_align(view.captions[k],LV_TEXT_ALIGN_CENTER,0);
        lv_obj_add_event_cb(b,choose,LV_EVENT_CLICKED,reinterpret_cast<void*>(static_cast<uintptr_t>(k)));}
    view.pagination=label(parent,202,212,28,"",muted);
    for(int k=0;k<4;++k){auto& row=view.rows[k];const int y=48+k*41;
        row.symbol=label(parent,10,y+17-(ct_font_ui_16.line_height-ct_font_ui_16.base_line),80,"--");lv_label_set_long_mode(row.symbol,LV_LABEL_LONG_SCROLL_CIRCULAR);lv_obj_set_style_text_font(row.symbol,&ct_font_ui_16,0);
        row.detail=label(parent,10,y+12,80,"",muted);lv_label_set_long_mode(row.detail,LV_LABEL_LONG_SCROLL_CIRCULAR);
        row.price=label(parent,153,y+17-(ct_font_ui_16.line_height-ct_font_ui_16.base_line),77,"--");lv_obj_set_style_text_font(row.price,&ct_font_ui_16,0);lv_obj_set_style_text_align(row.price,LV_TEXT_ALIGN_RIGHT,0);
        lv_label_set_long_mode(row.price,LV_LABEL_LONG_SCROLL_CIRCULAR);
        row.change=label(parent,153,y+15,77,"--");lv_obj_set_style_text_align(row.change,LV_TEXT_ALIGN_RIGHT,0);
        row.plot=lv_obj_create(parent);lv_obj_remove_style_all(row.plot);lv_obj_set_pos(row.plot,94,y+8);lv_obj_set_size(row.plot,56,32);
        lv_obj_remove_flag(row.plot,LV_OBJ_FLAG_CLICKABLE);lv_obj_remove_flag(row.plot,LV_OBJ_FLAG_SCROLLABLE);
        lv_obj_add_event_cb(row.plot,draw,LV_EVENT_DRAW_MAIN,&row);
        if(k<3){auto* rule=lv_obj_create(parent);lv_obj_remove_style_all(rule);lv_obj_set_pos(rule,10,y+38);lv_obj_set_size(rule,220,1);lv_obj_set_style_bg_color(rule,lv_color_hex(0x292F2E),0);lv_obj_set_style_bg_opa(rule,LV_OPA_COVER,0);lv_obj_remove_flag(rule,LV_OBJ_FLAG_CLICKABLE);}
    }
    view.status=label(parent,10,212,190,"",muted);selection();
}
void stock_view_update(const Snapshot& snapshot,uint64_t nowMs,Language selected){
    if(!view.status)return;
    language=selected;view.range=snapshot.stockChartRange;selection();
    const auto* stocks=!snapshot.stocks.unknown && snapshot.stocks.data?&*snapshot.stocks.data:nullptr;
    const std::size_t count=stocks?std::min<std::size_t>(stocks->count,5):0;
    const std::size_t first=count>4?((nowMs/6000)%2)*4:0;
    text(view.pagination,count>4?(first?"2/2":"1/2"):"");
    bool stale=snapshot.stocks.stale || (stocks && stocks->partialFailure);
    for(std::size_t k=0;k<4;++k){auto& row=view.rows[k];const Stock* stock=first+k<count?&stocks->items[first+k]:nullptr;
        row.color=stock && stock->changePercent && *stock->changePercent<0?copper:mint;
        text(row.symbol,stock?stock->symbol:"");text(row.detail,stock?(stock->stale?stock->currency+tr(" เก่า"," Stale"):stock->currency):"");
        text(row.price,stock?number(stock->price):"");text(row.change,stock?number(stock->changePercent,true):"");
        lv_obj_set_style_text_color(row.change,lv_color_hex(row.color),0);chart(row,stock);stale=stale || (stock && stock->stale);
    }
    std::string status;
    if(view.failed)status=tr("บันทึกไม่สำเร็จ","Could not save");
    else if(!snapshot.stocksConfigured)status=tr("ตั้งค่ารายการหุ้น","Configure stocks");
    else if(!count)status=tr("ยังไม่มีข้อมูล","No data");
    else status=std::string(stale?tr("ข้อมูลเก่า ","Stale "):tr("รับแล้ว ","Received "))+std::to_string((nowMs>=snapshot.stocks.receivedAtMs?nowMs-snapshot.stocks.receivedAtMs:0)/60000)+tr(" น.","m");
    text(view.status,status);
}
}
