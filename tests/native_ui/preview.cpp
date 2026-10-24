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
std::time_t wallEpoch=-1;
extern "C" std::time_t time(std::time_t* out) {timespec real{};clock_gettime(CLOCK_REALTIME,&real);auto result=wallEpoch<0?real.tv_sec:wallEpoch;if(out)*out=result;return result;}
namespace deskduo {
Config cfg;
Snapshot data;
CodexUsageMode mode=CodexUsageMode::Both;
Language language=Language::Thai;
uint8_t brightness=100, hardwareBrightness=100;
unsigned hardwareCalls=0, admissions=0, persistence=0;
bool hardwareFail=false, saveFail=false, brightnessAuto=false;
uint16_t brightnessDayStart=360,brightnessNightStart=1080;
std::optional<uint8_t> backlight_last_applied_percent(){return hardwareBrightness;}
std::vector<unsigned> hardwareLevels;
std::optional<uint8_t> failLevel;
esp_err_t display_set_brightness(uint8_t level){++hardwareCalls;hardwareLevels.push_back(level);if(hardwareFail || (failLevel && *failLevel==level))return ESP_FAIL;hardwareBrightness=level;return ESP_OK;}
unsigned stockSaves=0;
int runtime_patch(const std::string& body){
 assert(!maintenance.acquire(Maintenance::Settings));
 assert(body=="{\"stocks\":{\"chartRange\":\"1d\"}}" || body=="{\"stocks\":{\"chartRange\":\"5d\"}}");
 ++stockSaves;if(saveFail)return 503;
 cfg.stocks.chartRange=body.find("5d")!=std::string::npos?StockChartRange::FiveDays:StockChartRange::Day;
 data.stockChartRange=cfg.stocks.chartRange;return 204;
}
int runtime_display_patch(const std::string& body){
 ++admissions;assert(!maintenance.acquire(Maintenance::Settings) && "local save must hold shared Settings lease");
 if(body.find("brightnessAuto")!=std::string::npos){if(saveFail)return 503;brightnessAuto=body.find("true")!=std::string::npos;++persistence;return 204;}
 if(body.find("language")!=std::string::npos){if(saveFail)return 503;language=body.find("en")!=std::string::npos?Language::English:Language::Thai;return 204;}
 assert(body.find("codexUsageMode")==std::string::npos);
 if(saveFail)return 503;
 auto start=body.find(':');assert(start!=std::string::npos);brightness=std::stoi(body.substr(start+1));++persistence;return 204;
}
Language runtime_language(){return language;}
DisplaySettings runtime_display_settings(){return {mode,language,brightness,brightnessAuto,brightnessDayStart,brightnessNightStart};}
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
// Repeated window names belong to separate providers. Find by the visible
// heading band so Weekly-only assertions cannot accidentally inspect Claude.
lv_area_t labelInkBounds(lv_obj_t* label);
lv_obj_t* providerLabel(const char* provider,const char* value,bool includeHidden=false){
    auto* heading=findLabel(lv_screen_active(),provider);assert(heading);
    auto* root=lv_obj_get_parent(heading);
    lv_obj_update_layout(root);lv_area_t bounds{};lv_obj_get_coords(root,&bounds);
    int divider=-1;
    for(uint32_t i=0;i<lv_obj_get_child_count(root);++i){auto* obj=lv_obj_get_child(root,i);if(lv_obj_get_height(obj)==1 && lv_obj_get_width(obj)==220)divider=lv_obj_get_y(obj);}
    assert(divider>=0);
    const int top=std::strcmp(provider,"Claude")==0?0:divider;
    const int bottom=std::strcmp(provider,"Claude")==0?divider:lv_obj_get_height(root);
    for(uint32_t i=0;i<lv_obj_get_child_count(root);++i){
        auto* obj=lv_obj_get_child(root,i);
        if(!lv_obj_check_type(obj,&lv_label_class))continue;
        if(!includeHidden && lv_obj_has_flag(obj,LV_OBJ_FLAG_HIDDEN))continue;
        if(std::strcmp(lv_label_get_text(obj),value)!=0)continue;
        const auto ink=labelInkBounds(obj);const int center=(ink.y1+ink.y2)/2-bounds.y1;
        if(center>=top && center<bottom)return obj;
    }
    return nullptr;
}
// Measure actual embedded glyph boxes including LVGL's right alignment.
lv_area_t labelInkBounds(lv_obj_t* label){
    assert(label);lv_obj_update_layout(lv_screen_active());
    lv_area_t area{};lv_obj_get_coords(label,&area);
    lv_area_t ink{240,320,-1,-1};
    auto* font=lv_obj_get_style_text_font(label,0);const char* value=lv_label_get_text(label);
    uint32_t offset=0,index=0;
    while(value[offset]){
        const auto cp=lv_text_encoded_next(value,&offset);lv_font_glyph_dsc_t g{};
        assert(lv_font_get_glyph_dsc(font,&g,cp,0) && !g.is_placeholder);
        lv_point_t pos{};lv_label_get_letter_pos(label,index++,&pos);
        if(!g.box_w || !g.box_h)continue;
        const int x=area.x1+pos.x+g.ofs_x;
        const int y=area.y1+pos.y+font->line_height-font->base_line-g.ofs_y-g.box_h;
        ink.x1=std::min(ink.x1,x);ink.x2=std::max(ink.x2,x+g.box_w-1);
        ink.y1=std::min(ink.y1,y);ink.y2=std::max(ink.y2,y+g.box_h-1);
    }
    assert(ink.x2>=ink.x1);return ink;
}
void assertUsageColumnGap(lv_obj_t* window,lv_obj_t* percent){
    const auto a=labelInkBounds(window),b=labelInkBounds(percent);
    const int gap=b.x1-a.x2-1;
    if(gap<8)std::cerr<<"Crowded usage columns: "<<lv_label_get_text(window)<<" / "<<lv_label_get_text(percent)<<" gap="<<gap<<"\n";
    assert(gap>=8 && "window label and percentage need eight clear pixels");
    assert(b.x2<230 && "right-aligned percentage must fit the usage column");
}
void assertDuoVerticalSpacing(){
    lv_obj_update_layout(lv_screen_active());
    auto* claude=findLabel(lv_screen_active(),"Claude");auto* codex=findLabel(lv_screen_active(),"Codex");assert(claude && codex);
    auto* root=lv_obj_get_parent(claude);lv_obj_t *divider=nullptr,*lastClaudeBar=nullptr,*lastBar=nullptr,*footer=nullptr;
    const auto codexInk=labelInkBounds(codex);
    for(uint32_t i=0;i<lv_obj_get_child_count(root);++i){
        auto* obj=lv_obj_get_child(root,i);if(lv_obj_has_flag(obj,LV_OBJ_FLAG_HIDDEN))continue;
        if(lv_obj_get_height(obj)==1 && lv_obj_get_width(obj)==220)divider=obj;
        if(lv_obj_check_type(obj,&lv_bar_class)){
            if(lv_obj_get_y(obj)<lv_obj_get_y(codex) && (!lastClaudeBar || lv_obj_get_y(obj)>lv_obj_get_y(lastClaudeBar)))lastClaudeBar=obj;
            if(!lastBar || lv_obj_get_y(obj)>lv_obj_get_y(lastBar))lastBar=obj;
        }
    }
    assert(divider && lastClaudeBar && lastBar);
    lv_area_t a{},d{},b{};lv_obj_get_coords(lastClaudeBar,&a);lv_obj_get_coords(divider,&d);lv_obj_get_coords(lastBar,&b);
    assert(d.y1-a.y2-1>=6 && "Claude usage bar needs breathing room before divider");
    assert(codexInk.y1-d.y2-1>=6 && "Codex heading needs breathing room after divider");
    for(uint32_t i=0;i<lv_obj_get_child_count(root);++i){
        auto* obj=lv_obj_get_child(root,i);
        if(lv_obj_check_type(obj,&lv_label_class) && !lv_obj_has_flag(obj,LV_OBJ_FLAG_HIDDEN) && lv_obj_get_y(obj)>lv_obj_get_y(lastBar))footer=obj;
    }
    assert(footer);const auto ink=labelInkBounds(footer);
    if(ink.y1-b.y2-1<6)std::cerr<<"Reset gap: "<<lv_label_get_text(footer)<<" ink="<<ink.y1<<".."<<ink.y2<<" bar="<<b.y1<<".."<<b.y2<<" gap="<<ink.y1-b.y2-1<<"\n";
    assert(ink.y1-b.y2-1>=6 && "usage bars and reset/status footer must remain distinct");
    assert(256-ink.y2-1>=4 && "status footer needs space above the fixed IP divider");
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
    lv_obj_t* tab=nullptr;
    for(uint32_t k=0;k<lv_obj_get_child_count(lv_screen_active());++k){
        auto* candidate=lv_obj_get_child(lv_screen_active(),k);
        if(lv_obj_get_y(candidate)==276 && (tab=findLabel(candidate,shaped.c_str())))break;
    }
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
void autoBrightnessChecks(){
 using namespace deskduo;
 setenv("TZ","ICT-7",1);tzset();
 auto clock=[](int hour,int minute=0){std::tm t{};t.tm_year=126;t.tm_mon=9;t.tm_mday=16;t.tm_hour=hour;t.tm_min=minute;wallEpoch=std::mktime(&t);};
 brightnessAuto=true;brightness=51;language=Language::English;clock(12);ui_tick();tapTab("ตั้งค่า");
 auto slider=[](){auto* s=findSlider(lv_screen_active());assert(s);return s;};
 auto* existing=slider();auto saved=persistence;auto writes=admissions;
 assert(hardwareBrightness==51);clock(18);ui_tick();assert(hardwareBrightness==26 && brightness==51 && persistence==saved && admissions==writes && slider()==existing);
 clock(6);ui_tick();assert(hardwareBrightness==51 && brightness==51 && persistence==saved);
 wallEpoch=0;ui_tick();assert(hardwareBrightness==51);clock(22);ui_tick();assert(hardwareBrightness==26 && persistence==saved);
 assert(findLabel(lv_screen_active(),"51%"));clockMs+=1000;capture("auto-brightness-setup-en.ppm");
 lv_obj_update_layout(slider());lv_area_t b{};lv_obj_get_coords(slider(),&b);const int y=(b.y1+b.y2)/2;
 auto press=[&](int x){pointerAt(x,y,true);};auto release=[&](int x){pointerAt(x,y,false);};
 auto drag=[&](int x){press((b.x1+b.x2)/2);press(x);};
 // Actual night drag previews the derived half. Clock/web ticks cannot stomp it.
 drag(b.x2);assert(hardwareBrightness==50 && brightness==51 && persistence==saved);
 brightness=25;brightnessDayStart=1200;brightnessNightStart=360;ui_tick();assert(hardwareBrightness==50);
 release(b.x2);assert(brightness==100 && brightnessAuto && brightnessDayStart==1200 && brightnessNightStart==360 && hardwareBrightness==100 && persistence==saved+1 && admissions==writes+1);
 saved=persistence;writes=admissions;brightnessDayStart=360;brightnessNightStart=1080;brightness=51;clock(17,59);ui_tick();
 drag(b.x2);assert(hardwareBrightness==100);clock(18);ui_tick();assert(hardwareBrightness==100 && persistence==saved);release(b.x2);
 assert(hardwareBrightness==50 && brightness==100 && persistence==saved+1 && admissions==writes+1);
 saved=persistence;writes=admissions;brightness=51;ui_tick();
 // At a held boundary the required release application must succeed before save.
 clock(17,59);ui_tick();drag(b.x2);clock(18);failLevel=50;release(b.x2);failLevel.reset();
 assert(brightness==51 && hardwareBrightness==26 && persistence==saved && admissions==writes && findLabel(lv_screen_active(),"Brightness failed"));
 // Latest derived committed state is restored by all existing cancellation paths.
 drag(b.x1);brightness=75;lv_indev_reset(fakePointer,slider());release(b.x1);assert(hardwareBrightness==38 && brightness==75 && persistence==saved);
 drag(b.x1);brightness=99;lv_indev_wait_release(fakePointer);release(b.x1);assert(hardwareBrightness==50 && persistence==saved);
 drag(b.x1);brightness=51;tapTab("คู่หู");release(b.x1);assert(hardwareBrightness==26 && !findSlider(lv_screen_active()) && persistence==saved);
 tapTab("ตั้งค่า");drag(b.x1);brightness=75;language=Language::Thai;ui_tick();release(b.x1);assert(hardwareBrightness==38 && persistence==saved);clockMs+=1000;capture("auto-brightness-setup-th.ppm");
 language=Language::English;ui_tick();drag(b.x1);brightness=51;buttonLevel=0;ui_tick();clockMs+=100;buttonLevel=1;ui_tick();release(b.x1);assert(hardwareBrightness==26 && !findSlider(lv_screen_active()) && persistence==saved);
 tapTab("ตั้งค่า");assert(maintenance.acquire(Maintenance::Update));drag(b.x1);release(b.x1);maintenance.release();assert(hardwareBrightness==26 && persistence==saved && admissions==writes);
 saveFail=true;drag(b.x2);release(b.x2);saveFail=false;assert(hardwareBrightness==26 && brightness==51 && persistence==saved && admissions==writes+1);
 writes=admissions;hardwareFail=true;drag(b.x1);release(b.x1);hardwareFail=false;ui_tick();assert(hardwareBrightness==26 && persistence==saved && admissions==writes);
 brightness=99;hardwareFail=true;ui_tick();assert(hardwareBrightness==26 && backlight_last_applied_percent()==26);hardwareFail=false;ui_tick();assert(hardwareBrightness==50 && backlight_last_applied_percent()==50 && persistence==saved);
 // Unknown/recovered/jumping clocks do not mutate the saved base or use NVS.
 for(int hour:{6,18,5,12,22}){clock(hour);ui_tick();assert(brightness==99 && persistence==saved && admissions==writes);}
 brightnessAuto=false;brightnessDayStart=360;brightnessNightStart=1080;brightness=100;wallEpoch=-1;ui_tick();tapTab("คู่หู");
 std::cout<<"Auto brightness actual LVGL pointer/clock: ticks, NTP, no writes/rebuild, night preview, concurrent schedule, held boundary success/failure, latest derived reset/lost/page/language/BOOT, busy/save/apply failure passed\n";
}
lv_obj_t* actualContentRoot();
void stockChecks(){
 using namespace deskduo;
 const auto original=data;language=Language::English;data.autoRotateSeconds=0;data.stocksConfigured=true;
 Stocks stocks;stocks.count=5;
 const char* symbols[]={"AAPL","NVDA","PTT.BK","MSFT","GOOG"};
 for(int k=0;k<5;++k){auto& st=stocks.items[k];st.symbol=symbols[k];st.label="Demo";st.currency="USD";st.price=100+k;st.changePercent=k%2?-1.5:2.5;st.historyCount=5;
 auto points=std::make_shared<std::array<StockPoint,32>>();for(int j=0;j<5;++j)(*points)[j]={1791097200+j*300,100.0+j};st.history=points;}
 data.stocks={stocks,clockMs,false,false};data.stockChartRange=StockChartRange::Day;ui_tick();
 tapTab("หุ้น");clockMs=60000;capture("stocks-four-en.ppm");
 auto* title=findLabel(lv_screen_active(),"Stocks");assert(title && lv_obj_get_parent(title)==lv_screen_active() && lv_obj_get_y(title)<24);
 assert(!findLabel(lv_screen_active(),"Desk Duo"));
 for(int k=0;k<4;++k)assert(findLabel(actualContentRoot(),symbols[k]));
 assert(!findLabel(actualContentRoot(),"GOOG"));
 auto* period=findLabel(actualContentRoot(),"5 days");assert(period);auto* button=lv_obj_get_parent(period);lv_obj_update_layout(button);
 lv_area_t b{};lv_obj_get_coords(button,&b);assert(b.y2-b.y1+1>=44 && b.x2-b.x1+1>=44);
 auto saves=stockSaves;pointerTapAt(b.x1+1,b.y1+1);assert(stockSaves==saves+1 && data.stockChartRange==StockChartRange::FiveDays);
 pointerTapAt(b.x2-1,b.y2-1);assert(stockSaves==saves+1);
 auto* today=lv_obj_get_parent(findLabel(actualContentRoot(),"Today"));lv_obj_get_coords(today,&b);
 saveFail=true;pointerTapAt(b.x1+1,b.y2-1);saveFail=false;assert(stockSaves==saves+2 && data.stockChartRange==StockChartRange::FiveDays);
 assert(findLabel(actualContentRoot(),"Could not save"));
 assert(maintenance.acquire(Maintenance::Update));pointerTapAt(b.x2-1,b.y1+1);maintenance.release();assert(stockSaves==saves+2);
 pointerTapAt(b.x2-1,b.y1+1);assert(stockSaves==saves+3 && data.stockChartRange==StockChartRange::Day);
 // Real RGB output must contain the chart, preserve a null gap, and draw a flat series.
 auto colored=[](int x,int y){auto* p=frame+(y*240+x)*3;return p[0]>120 && p[1]>150 && p[2]>120;};
 auto chartPixels=[&](){unsigned n=0;for(int y=80;y<112;++y)for(int x=94;x<150;++x)n+=colored(x,y);return n;};
 clockMs=60000;capture("stocks-chart-line.ppm");assert(chartPixels()>30);
 for(uint32_t k=0;k<lv_obj_get_child_count(actualContentRoot());++k)assert(!lv_obj_check_type(lv_obj_get_child(actualContentRoot(),k),&lv_image_class));
 data.stockChartRange=StockChartRange::FiveDays;clockMs+=1000;capture("stocks-chart-wrong-range.ppm");assert(chartPixels()==0);
 data.stockChartRange=StockChartRange::Day;

 for(auto& st:data.stocks.data->items){auto points=std::make_shared<std::array<StockPoint,32>>(*st.history);for(int j=0;j<5;++j)(*points)[j].close=100;st.history=points;}
 clockMs+=1000;capture("stocks-chart-flat.ppm");assert(chartPixels()>30);assert(colored(120,96));
 for(auto& st:data.stocks.data->items){auto points=std::make_shared<std::array<StockPoint,32>>(*st.history);(*points)[2].close.reset();st.history=points;}
 clockMs+=1000;capture("stocks-chart-gap.ppm");assert(!colored(120,96) && chartPixels()>10);
 for(auto& st:data.stocks.data->items){auto points=std::make_shared<std::array<StockPoint,32>>(*st.history);for(auto& p:*points)p.close.reset();st.history=points;}
 clockMs+=1000;capture("stocks-chart-all-null.ppm");assert(chartPixels()==0);
 for(auto& st:data.stocks.data->items)st.historyCount=0;
 clockMs=60000;capture("stocks-chart-empty.ppm");assert(chartPixels()==0);
 data.stocks.data->items[0].stale=true;clockMs+=1000;capture("stocks-partial-stale.ppm");assert(findLabel(actualContentRoot(),"USD Stale"));
 clockMs=66000;capture("stocks-fifth.ppm");assert(findLabel(actualContentRoot(),"GOOG"));assert(!findLabel(actualContentRoot(),"AAPL"));
 data.stocks.data=stocks;language=Language::Thai;clockMs=60000;capture("stocks-four-th.ppm");assert(findLabel(lv_screen_active(),shapeText("หุ้น").c_str()));assert(findLabel(actualContentRoot(),shapeText("วันนี้").c_str()));
 data.stocks.data.reset();data.stocks.unknown=true;clockMs+=1000;capture("stocks-no-data.ppm");assert(findLabel(actualContentRoot(),shapeText("ยังไม่มีข้อมูล").c_str()));
 data.stocksConfigured=false;clockMs+=1000;capture("stocks-unconfigured.ppm");assert(findLabel(actualContentRoot(),shapeText("ตั้งค่ารายการหุ้น").c_str()));
 tapTab("คู่หู");assert(findLabel(lv_screen_active(),shapeText(language==Language::English?"Duo":"คู่หู").c_str()));data=original;
 std::cout<<"Stocks: localized header, four rows, fifth pagination, real 44px period pointers, leased sparse save/busy/failure, line/flat/gap/empty RGB passed\n";
}

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
 assert(findLabel(lv_screen_active(),"Language"));assert(lv_obj_get_scroll_y(lv_obj_get_parent(s))==0);
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
lv_obj_t* actualContentRoot(){
    auto* screen=lv_screen_active();lv_obj_update_layout(screen);
    lv_obj_t* root=nullptr;unsigned matches=0;
    for(uint32_t i=0;i<lv_obj_get_child_count(screen);++i){
        auto* child=lv_obj_get_child(screen,i);
        if(lv_obj_get_x(child)==0 && lv_obj_get_y(child)==24 && lv_obj_get_width(child)==240 && lv_obj_get_height(child)==232){root=child;++matches;}
    }
    assert(matches==1 && "actual UI content viewport must exist uniquely");return root;
}
lv_obj_t* findWeatherLabel(const char* value){
    return findLabel(actualContentRoot(),deskduo::shapeText(value).c_str());
}
lv_obj_t* weatherLabel(const char* value){
    auto* obj=findWeatherLabel(value);
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
    if(natural.y>lv_obj_get_style_text_font(obj,0)->line_height || lv_obj_get_height(obj)>lv_obj_get_style_text_font(obj,0)->line_height)std::cerr<<"BASELINE "<<lv_label_get_text(obj)<<" natural "<<natural.y<<" height "<<lv_obj_get_height(obj)<<" font "<<lv_obj_get_style_text_font(obj,0)->line_height<<"\n";
    assert(natural.y<=lv_obj_get_style_text_font(obj,0)->line_height &&
           lv_obj_get_height(obj)<=lv_obj_get_style_text_font(obj,0)->line_height && "weather reading must stay on one baseline");
    lv_area_t box{};lv_obj_get_coords(obj,&box);
    if(!(box.x1>=0 && box.x2<240 && box.y1>=24 && box.y2<256))std::cerr<<"Out of bounds "<<lv_label_get_text(obj)<<" "<<box.x1<<","<<box.y1<<","<<box.x2<<","<<box.y2<<"\n";
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
    assert(lv_obj_get_y(label)+font->line_height-font->base_line<=lv_obj_get_y(bar) && "reset baseline must stay in its reserved usage row after language rebuild");
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
void setupLanguageChecks(){
 using namespace deskduo;
 language=Language::English;ui_tick();tapTab("ตั้งค่า");
 auto choose=[](const char* name){auto* caption=findLabel(actualContentRoot(),name);assert(caption);auto* button=lv_obj_get_parent(caption);lv_obj_update_layout(button);lv_area_t b{};lv_obj_get_coords(button,&b);assert(b.y2-b.y1+1>=44);pointerTapAt((b.x1+b.x2)/2,(b.y1+b.y2)/2);};
 choose("TH");assert(language==Language::Thai);assert(findLabel(actualContentRoot(),shapeText("ภาษา").c_str()));
 choose("EN");assert(language==Language::English);
 saveFail=true;choose("TH");saveFail=false;assert(language==Language::English && findLabel(actualContentRoot(),"Could not save"));
 tapTab("คู่หู");tapTab("ตั้งค่า");
 brightness=60;brightnessAuto=true;brightnessDayStart=370;brightnessNightStart=1110;clockMs+=1000;ui_tick();
 assert(findLabel(actualContentRoot(),"Scheduled brightness"));assert(findLabel(actualContentRoot(),"Day             06:10"));assert(findLabel(actualContentRoot(),"Night           18:30"));
 capture("setup-schedule-language-en.ppm");
 brightnessAuto=false;brightnessDayStart=360;brightnessNightStart=1080;
 std::cout<<"Setup: actual 44px TH/EN language pointers, saved readback, failed save and schedule readback passed\n";
}
void weatherChecks(){
 using namespace deskduo;
 const auto original=data.weather;const auto place=data.weatherLabel;const auto savedWall=wallEpoch;
 language=Language::English;ui_tick();tapTab("อากาศ");
 auto& w=*data.weather.data;w.updatedAt=1791097200;w.isDay=true;w.weatherCode=2;wallEpoch=*w.updatedAt+900;
 for(int k=0;k<4;++k){w.hourlyWeatherCodes[k]=k==1?63:2;w.hourlyIsDay[k]=true;}
 clockMs+=1000;capture("weather-day-en.ppm");weatherLabel("Data 14:00");weatherLabel("31.0");
 auto images=[](){unsigned n=0;auto* root=actualContentRoot();for(uint32_t k=0;k<lv_obj_get_child_count(root);++k){auto* obj=lv_obj_get_child(root,k);if(lv_obj_check_type(obj,&lv_image_class) && !lv_obj_has_flag(obj,LV_OBJ_FLAG_HIDDEN))++n;}return n;};
 assert(images()==4 && "hero and next three forecasts must use bounded raster artwork");
 const auto day=regionHash(147,47,88,88);w.isDay=false;clockMs+=1000;capture("weather-night-en.ppm");
 assert(regionHash(147,47,88,88)!=day && "night needs a moon, distinct from daytime sun");
 w.isDay.reset();clockMs+=1000;capture("weather-day-unknown.ppm");assert(images()==4 && "missing day partial must use neutral cloud, never sun or moon");
 w.weatherCode=63;data.weather.stale=true;clockMs+=1000;capture("weather-rain-stale-en.ppm");weatherLabel("Stale 14:00");assert(images()==4);
 data.weather.receivedAtMs=clockMs;clockMs+=1000;ui_tick();weatherLabel("Stale 14:00");
 // Source-age policy is independent of newly received old observations.
 data.weather.stale=false;wallEpoch=*w.updatedAt+7199;clockMs+=1000;ui_tick();weatherLabel("Data 14:00");
 wallEpoch=*w.updatedAt+7200;data.weather.receivedAtMs=clockMs;clockMs+=1000;ui_tick();weatherLabel("Stale 14:00");
 wallEpoch=*w.updatedAt+172800;data.weather.receivedAtMs=clockMs;clockMs+=1000;capture("weather-old-observation-en.ppm");weatherLabel("Stale 04/10 14:00");
 language=Language::Thai;clockMs+=1000;capture("weather-old-observation-th.ppm");assertTextFits(weatherLabel("ข้อมูลเก่า 04/10 14:00"));
 language=Language::English;wallEpoch=0;clockMs+=1000;ui_tick();weatherLabel("Data 04/10 14:00");
 wallEpoch=*w.updatedAt+900;
 // A newly received old observation stays the source's time, never receipt freshness.
 data.weather.stale=false;w.updatedAt.reset();clockMs+=1000;ui_tick();weatherLabel("Data --");
 for(auto selected:{Language::Thai,Language::English}) {
   language=selected;w.updatedAt=1791097200;clockMs+=1000;capture(selected==Language::English?"weather-rain-en.ppm":"weather-rain-th.ppm");
   auto* root=actualContentRoot();assert(!lv_obj_has_flag(root,LV_OBJ_FLAG_SCROLLABLE));
   for(uint32_t k=0;k<lv_obj_get_child_count(root);++k){auto* obj=lv_obj_get_child(root,k);if(lv_obj_check_type(obj,&lv_label_class)){if(lv_label_get_long_mode(obj)!=LV_LABEL_LONG_SCROLL_CIRCULAR)assertTextFits(obj);}}
 }
 language=Language::English;clockMs+=1000;ui_tick();
 struct Timezone {int64_t epoch;int32_t offset;std::array<const char*,3> times;};
 for(const auto& t:std::array<Timezone,4>{{{1791153900,19800,{"04:15","05:15","06:15"}},{1791162000,-18000,{"20:00","21:00","22:00"}},{1791154800,0,{"23:00","00:00","01:00"}},{1791162000,-12600,{"21:30","22:30","23:30"}}}}) {
   w.utcOffsetSeconds=t.offset;w.updatedAt=t.epoch;
   for(int k=0;k<4;++k)w.hourlyTimes[k]=t.epoch+k*3600;
   clockMs+=1000;ui_tick();for(int k=0;k<3;++k)assertTextFits(weatherLabel(t.times[k]));
 }
 // Calendar comparison must use the feed's local day, not device Bangkok.
 w.updatedAt=1791154800;w.utcOffsetSeconds=-18000;wallEpoch=*w.updatedAt+3600;clockMs+=1000;ui_tick();weatherLabel("Data 18:00");
 w.utcOffsetSeconds=19800;clockMs+=1000;ui_tick();weatherLabel("Data 04:30");
 w.updatedAt=1791158100;w.utcOffsetSeconds=0;wallEpoch=*w.updatedAt+900;clockMs+=1000;ui_tick();weatherLabel("Data 04/10 23:55");
 // A future timestamp is not an old observation after clock correction.
 wallEpoch=*w.updatedAt-3600;clockMs+=1000;ui_tick();weatherLabel("Data 23:55");
 w.utcOffsetSeconds.reset();clockMs+=1000;ui_tick();weatherLabel("Data --");assert(!findWeatherLabel("21:30"));
 w.hourlyTemperatures[0]=0;w.hourlyRainProbability[0]=0;clockMs+=1000;ui_tick();weatherLabel("0.0°");weatherLabel("0%");
 w.hourlyCount=0;clockMs+=1000;ui_tick();assert(images()==1 && !findWeatherLabel("0.0°"));
 for(double value:{-999.99,999.99,1e300,-1e300}){w.temperature=value;clockMs+=1000;ui_tick();assertTextFits(weatherLabel(value==-999.99?"-1000.0":value==999.99?"1000.0":"Out of range"));}
 data.weather.unknown=true;clockMs+=1000;capture("weather-unknown.ppm");weatherLabel("No data");assert(images()==0);
 data.weather=original;data.weatherLabel=place;wallEpoch=savedWall;clockMs+=1000;ui_tick();
 std::cout<<"Weather: raster day/night/rain, missing day/code, source observation/stale, next three forecasts, TH/EN bounds, source offsets/rollover, zero/null/extremes passed\n";
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
        auto* reset=providerLabel("Codex",shapeText(english?"Weekly":"สัปดาห์").c_str());
        assert(reset);assertGlyphsAboveBar(reset);assertUsageColumnGap(reset,providerLabel("Codex","21%"));
        language=english?Language::Thai:Language::English;clockMs+=20;ui_tick();
        assert(findLabel(lv_screen_active(),"21%"));
        assert(findLabel(lv_screen_active(),shapeText(english?"สัปดาห์":"Weekly").c_str()));
        language=selected;clockMs+=20;ui_tick();
        data.claude.stale=true;data.claude.receivedAtMs=clockMs;clockMs+=1000;ui_tick();
        assert(findLabel(lv_screen_active(),shapeText(english?"stale 0m":"เก่า 0น.").c_str()));
        mode=CodexUsageMode::Both;clockMs+=1000;
        capture(("language-usage-both-"+suffix+".ppm").c_str());
        assert(findLabel(lv_screen_active(),"58%"));
        tapTab("อากาศ");capture(("language-weather-"+suffix+".ppm").c_str());
        assertTextFits(weatherLabel(english?"Partly cloudy":"มีเมฆบางส่วน"));
        assertTextFits(weatherLabel(english?"Humidity":"ความชื้น"));
        assertTextFits(weatherLabel(english?"Next 3 hours":"3 ชั่วโมงถัดไป"));
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
        assertTextFits(weatherLabel(english?"Stale --":"ข้อมูลเก่า --"));
        for(uint64_t elapsed:{59ULL*60000,999ULL*3600000,2000ULL*3600000}) {
            clockMs=data.weather.receivedAtMs+elapsed;ui_tick();lv_obj_update_layout(lv_screen_active());
            auto* content=lv_obj_get_parent(weatherLabel("31.0"));
            for(uint32_t k=0;k<lv_obj_get_child_count(content);++k){
                auto* label=lv_obj_get_child(content,k);
                if(lv_obj_check_type(label,&lv_label_class) && lv_obj_get_y(label)==104)assertTextFits(label);
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
        assert(findLabel(lv_screen_active(),shapeText(english?"Stocks":"หุ้น").c_str()));
        assert(findLabel(lv_screen_active(),"USD"));
        tapTab("ตั้งค่า");capture(("language-setup-"+suffix+".ppm").c_str());
        assert(findLabel(lv_screen_active(),shapeText(english?"Language":"ภาษา").c_str()));
        assert(findLabel(lv_screen_active(),"hidden-new-token"));
        language=english?Language::Thai:Language::English;clockMs+=10;ui_tick();
        assert(findLabel(lv_screen_active(),"hidden-new-token"));
        assert(findLabel(lv_screen_active(),shapeText(english?"ภาษา":"Language").c_str()));
        language=selected;clockMs+=10;ui_tick();
        for(const char* tab:{"คู่หู","อากาศ","หุ้น"}) {
            tapTab(tab);assert(!findLabel(lv_screen_active(),"hidden-new-token"));
        }
        const char* markers[]={"Claude","°C",english?"Stocks":"หุ้น",english?"Language":"ภาษา"};
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
void scheduleSwitchChecks(){
 using namespace deskduo;
 language=Language::English;brightness=63;brightnessAuto=false;brightnessDayStart=375;brightnessNightStart=1125;ui_tick();tapTab("ตั้งค่า");
 auto find=[](auto&& self,lv_obj_t* parent)->lv_obj_t*{if(lv_obj_check_type(parent,&lv_switch_class))return parent;for(uint32_t k=0;k<lv_obj_get_child_count(parent);++k)if(auto* s=self(self,lv_obj_get_child(parent,k)))return s;return nullptr;};
 auto* sw=find(find,actualContentRoot());assert(sw && !lv_obj_has_flag(sw,LV_OBJ_FLAG_CLICKABLE));auto* target=lv_obj_get_parent(sw);lv_obj_update_layout(target);lv_area_t b;lv_obj_get_coords(target,&b);assert(b.x2-b.x1+1>=44 && b.y2-b.y1+1>=44);
 auto saved=persistence,writes=admissions;assert(!lv_obj_has_state(sw,LV_STATE_CHECKED));pointerTapAt(b.x1+1,b.y1+1);ui_tick();assert(brightnessAuto && persistence==saved+1 && admissions==writes+1 && lv_obj_has_state(sw,LV_STATE_CHECKED));assert(brightness==63 && brightnessDayStart==375 && brightnessNightStart==1125);
 saved=persistence;writes=admissions;assert(maintenance.acquire(Maintenance::Update));pointerTapAt(b.x2-1,b.y2-1);maintenance.release();assert(brightnessAuto && persistence==saved && admissions==writes && lv_obj_has_state(sw,LV_STATE_CHECKED));
 saveFail=true;pointerTapAt(b.x2-1,b.y1+1);saveFail=false;ui_tick();assert(brightnessAuto && persistence==saved && admissions==writes+1 && lv_obj_has_state(sw,LV_STATE_CHECKED));assert(findLabel(actualContentRoot(),"Could not save"));
 pointerTapAt(b.x1+1,b.y2-1);ui_tick();assert(!brightnessAuto && persistence==saved+1 && !lv_obj_has_state(sw,LV_STATE_CHECKED));assert(brightness==63 && brightnessDayStart==375 && brightnessNightStart==1125);
 brightnessAuto=true;clockMs+=1000;ui_tick();assert(lv_obj_has_state(sw,LV_STATE_CHECKED));
 brightnessAuto=false;brightness=100;brightnessDayStart=360;brightnessNightStart=1080;tapTab("คู่หู");
 std::cout<<"Setup schedule switch:44pxtarget, sharedlease/sparsepatch, busy/savefailure, readback and preservedbase/scheduletimes passed\n";
}
void assertHeaderInk(){
 lv_obj_update_layout(lv_screen_active());auto* screen=lv_screen_active();lv_obj_t* title=nullptr;
 for(uint32_t k=0;k<lv_obj_get_child_count(screen);++k){auto* o=lv_obj_get_child(screen,k);if(lv_obj_check_type(o,&lv_label_class) && lv_obj_get_x(o)==10){title=o;break;}}
 assert(title);const auto* font=lv_obj_get_style_text_font(title,0);const char* value=lv_label_get_text(title);uint32_t offset=0;int top=320,bottom=-1;
 while(value[offset]){auto cp=lv_text_encoded_next(value,&offset);lv_font_glyph_dsc_t g{};assert(lv_font_get_glyph_dsc(font,&g,cp,0) && !g.is_placeholder);if(!g.box_h)continue;int y=lv_obj_get_y(title)+font->line_height-font->base_line-g.ofs_y-g.box_h;
 const auto* descriptor=static_cast<const lv_font_fmt_txt_dsc_t*>(font->dsc);const auto* pixels=descriptor->glyph_bitmap+descriptor->glyph_dsc[g.gid.index].bitmap_index;
 for(unsigned row=0;row<g.box_h;++row)for(unsigned x=0;x<g.box_w;++x){const unsigned bit=(row*g.box_w+x)*descriptor->bpp;const auto opacity=(pixels[bit/8]>>(8-descriptor->bpp-bit%8))&((1U<<descriptor->bpp)-1);if(opacity){top=std::min(top,y+static_cast<int>(row));bottom=std::max(bottom,y+static_cast<int>(row));}}
}
 if(top<0 || bottom>=29)std::cerr<<"Title y="<<lv_obj_get_y(title)<<" ascent="<<font->line_height-font->base_line<<" ";
 if(top<0 || bottom>=29)std::cerr<<"Header ink bounds "<<value<<" "<<top<<".."<<bottom<<"\n";
 assert(top>=0 && bottom<29 && "all header marks must stay in y0..28 above first content pixels");
 std::cout<<"Header ink fit "<<value<<": "<<top<<".."<<bottom<<", UI"<<(font==&ct_font_ui_18?18:16)<<"\n";
}
void assertHeaderContentGap(){
 for(int x=0;x<240;++x)for(int c=0;c<3;++c)assert(frame[(29*240+x)*3+c]==0 && "y29 must separate full header ink from first content at y30/31");
}
void fidelityCaptures(){
 using namespace deskduo;
 data.autoRotateSeconds=0;mode=CodexUsageMode::Both;wallEpoch=1791124200;clockMs=60000;
 Usage a;a.primary=UsageWindow{38,300,1791130200};a.secondary=UsageWindow{64,10080,1791356400};data.claude={a,clockMs,false,false};
 Usage b;b.primary=UsageWindow{21,300,1791130200};b.secondary=UsageWindow{47,10080,1791356400};data.codex={b,clockMs,false,false};
 Weather w;w.temperature=29.4;w.humidity=74;w.weatherCode=2;w.isDay=false;w.updatedAt=wallEpoch-900;w.utcOffsetSeconds=25200;w.hourlyCount=3;
 for(int k=0;k<3;++k){w.hourlyTimes[k]=wallEpoch+1800+k*3600;w.hourlyTemperatures[k]=k==0?28.9:k==1?28.4:28.0;w.hourlyRainProbability[k]=k==0?35:k==1?50:40;w.hourlyWeatherCodes[k]=k?63:3;w.hourlyIsDay[k]=false;}
 data.weather={w,clockMs,false,false};data.weatherLabel="กรุงเทพฯ";
 Stocks stocks;stocks.count=4;const char* names[]={"PTT","ADVANC","AAPL","MSFT"};const double prices[]={32.50,285,224.80,428.20},changes[]={1.56,-0.70,0.84,-0.32};
 for(int k=0;k<4;++k){auto& st=stocks.items[k];st.symbol=names[k];st.currency=k<2?"THB":"USD";st.price=prices[k];st.changePercent=changes[k];st.historyCount=24;st.chartRange=StockChartRange::Day;auto pts=std::make_shared<std::array<StockPoint,32>>();for(int j=0;j<24;++j)(*pts)[j]={wallEpoch+j*300,prices[k]+(k%2?-1:1)*(j*0.03+std::sin(j*1.7)*0.12)};st.history=pts;}
 data.stocks={stocks,clockMs,false,false};data.stocksConfigured=true;data.stockChartRange=StockChartRange::Day;
 brightness=60;brightnessAuto=true;brightnessDayStart=360;brightnessNightStart=1080;cfg.ssid="Desk-WiFi";
 for(auto selected:{Language::Thai,Language::English}){
  language=selected;data.weatherLabel=selected==Language::Thai?"กรุงเทพฯ":"Bangkok";clockMs+=1000;ui_tick();const std::string suffix=selected==Language::Thai?"th":"en";
  tapTab("คู่หู");assertHeaderInk();capture(("device-duo-"+suffix+".ppm").c_str());assertHeaderContentGap();
  assertDuoVerticalSpacing();
  int titleTop=29,titleBottom=-1;for(int y=0;y<29;++y)for(int x=10;x<170;++x){const auto* p=&frame[(y*240+x)*3];if(std::max({p[0],p[1],p[2]})>120){titleTop=std::min(titleTop,y);titleBottom=std::max(titleBottom,y);}}assert(titleBottom-titleTop+1>=11 && titleBottom<=28 && "prominent header must have visible11pxglyphmass before content");
  auto* root=actualContentRoot();unsigned bars=0;for(uint32_t k=0;k<lv_obj_get_child_count(root);++k){auto* o=lv_obj_get_child(root,k);if(!lv_obj_check_type(o,&lv_bar_class))continue;lv_area_t r{};lv_obj_get_coords(o,&r);assert(r.x1==94 && r.x2==229 && r.y2-r.y1+1==8);assert(r.y1==55 || r.y1==109 || r.y1==167 || r.y1==221);++bars;}assert(bars==4);
  assert(findLabel(root,"38%") && findLabel(root,"21%"));
  const int palette[4][3]={{251,132,82},{232,156,120},{130,206,189},{141,207,194}};const int barYs[]={58,112,170,224};
  for(int k=0;k<4;++k)for(int c=0;c<3;++c)assert(std::abs(static_cast<int>(frame[(barYs[k]*240+100)*3+c])-palette[k][c])<=3 && "approved primary/secondary provider palette must survive actual LVGL render");
  const int track[]={37,41,45};for(int c=0;c<3;++c)assert(std::abs(static_cast<int>(frame[(58*240+220)*3+c])-track[c])<=3);

  tapTab("อากาศ");assertHeaderInk();capture(("device-weather-"+suffix+".ppm").c_str());assertHeaderContentGap();
  tapTab("หุ้น");assertHeaderInk();capture(("device-stocks-"+suffix+".ppm").c_str());assertHeaderContentGap();
  tapTab("ตั้งค่า");assertHeaderInk();capture(("device-setup-"+suffix+".ppm").c_str());assertHeaderContentGap();
 }
 language=Language::English;tapTab("อากาศ");
 for(int scene=0;scene<3;++scene){data.weather.data->isDay=scene==0;data.weather.data->weatherCode=scene==2?63:2;clockMs+=1000;capture(scene==0?"device-weather-day.ppm":scene==1?"device-weather-night.ppm":"device-weather-rain.ppm");int left=230,right=-1;for(int y=32;y<128;++y)for(int x=134;x<230;++x){const auto* p=&frame[(y*240+x)*3];if(std::max({p[0],p[1],p[2]})>32){left=std::min(left,x);right=std::max(right,x);}}assert(right-left+1>=80 && "96px weather hero must have at least80px useful cloud mass");}
 uint64_t clearNightHash=0;
 for(int scene=0;scene<3;++scene){
  data.weather.data->weatherCode=scene==1?1:0;data.weather.data->isDay=scene==2;clockMs+=1000;
  capture(scene==0?"device-weather-clear-night.ppm":scene==1?"device-weather-mainly-clear-night.ppm":"device-weather-clear-day.ppm");
  const auto hero=regionHash(134,32,96,96);if(scene==0)clearNightHash=hero;else if(scene==1)assert(hero==clearNightHash && "known clear and mainly-clear night must show the same true clear-night artwork");else assert(hero!=clearNightHash && "known clear day must remain distinct from clear night");
  assert(findLabel(actualContentRoot(),"Clear"));auto* neutral=findLabel(actualContentRoot(),"?");assert(neutral && lv_obj_has_flag(lv_obj_get_parent(neutral),LV_OBJ_FLAG_HIDDEN));
 }
 std::ofstream("device-clear-scenes.json")<<R"({"device-weather-clear-night":{"weatherCode":0,"isDay":false,"fixture":true},"device-weather-mainly-clear-night":{"weatherCode":1,"isDay":false,"fixture":true},"device-weather-clear-day":{"weatherCode":0,"isDay":true,"fixture":true}})";
 data.weather.data->weatherCode=0;data.weather.data->isDay.reset();clockMs+=1000;capture("device-weather-neutral.ppm");assert(findLabel(actualContentRoot(),"?"));
 data.weather.unknown=true;clockMs+=1000;capture("device-weather-unknown.ppm");assert(findLabel(actualContentRoot(),"No data"));
 std::cout<<"Fidelity: four136x8pairedusagebars at55/109/167/221, matched approvedfixtures TH/ENfourpages,96pxweathercloudmass>=80px, explicitneutralclear/dayunknown passed\n";
}
void duoSpacingChecks(){
 using namespace deskduo;
 const auto original=data;const auto oldLanguage=language;const auto oldMode=mode;
 data.autoRotateSeconds=0;
 for(auto selected:{Language::Thai,Language::English})for(auto selectedMode:{CodexUsageMode::Both,CodexUsageMode::Weekly}){
  language=selected;mode=selectedMode;clockMs+=1000;ui_tick();tapTab("คู่หู");
  const auto window=shapeText(selectedMode==CodexUsageMode::Weekly?(selected==Language::Thai?"สัปดาห์":"Weekly"):(selected==Language::Thai?"5ชม.":"5h"));
  for(double percent:{0.0,73.25,99.99,100.0}){
   Usage a;a.primary=UsageWindow{32,300,1791097200};a.secondary=UsageWindow{48,10080,1791356400};data.claude={a,clockMs,false,false};
   Usage b;b.primary=UsageWindow{percent,300,1791097200};b.secondary=UsageWindow{selectedMode==CodexUsageMode::Weekly?percent:47,10080,1791356400};data.codex={b,clockMs,false,false};
   clockMs+=1000;ui_tick();lv_obj_update_layout(lv_screen_active());
   const char* value=percent==0?"0%":percent==73.25?"73.25%":percent==99.99?"99.99%":"100%";
   auto* number=providerLabel("Codex",value);assert(number);assertTextFits(number);
   auto* title=providerLabel("Codex",window.c_str());assert(title);assertUsageColumnGap(title,number);assertDuoVerticalSpacing();
   assertGlyphsAboveBar(title);assertGlyphsAboveBar(number);
  }
  const std::string name=std::string("duo-spaced-")+(selectedMode==CodexUsageMode::Weekly?"weekly-":"both-")+(selected==Language::Thai?"th":"en")+".ppm";
  capture(name.c_str());
 }
 data=original;language=oldLanguage;mode=oldMode;clockMs+=1000;ui_tick();
 std::cout<<"Duo spacing: TH/EN Both/Weekly, 0/73.25/99.99/100%, >=8px column gap, >=6px provider/status separation and fixed IP/navigation passed\n";
}
void duoResetChecks(){
 using namespace deskduo;
 const auto original=data;const auto oldLanguage=language;const auto oldMode=mode;const auto oldClock=clockMs;
 data.autoRotateSeconds=0;
 auto reset=[&](const char* provider,const char* percent,const char* date){
  const auto value=shapeText(std::string(language==Language::Thai?"เริ่มใหม่ ":"Reset ")+date);
  auto* obj=providerLabel(provider,value.c_str());assert(obj && "each reset belongs to its provider and selected window");assertTextFits(obj);
  auto* usage=providerLabel(provider,percent);assert(usage);auto* bar=findBarBelow(usage);assert(bar);
  const auto ink=labelInkBounds(obj);lv_area_t b{};lv_obj_get_coords(bar,&b);
  assert(ink.x1>=94 && ink.x2<230 && ink.y1-b.y2-1>=6 && "reset text must sit in the same right-hand usage area below its own bar");
  assert(ink.y1-b.y2<=20 && "reset must remain adjacent to its own usage bar");
  auto* other=providerLabel(provider==std::string("Claude")?"Codex":"Claude",value.c_str());
  assert(!other && "distinct reset timestamps must never leak across providers");
 };
 for(auto selected:{Language::Thai,Language::English}){
  language=selected;mode=CodexUsageMode::Both;clockMs+=1000;ui_tick();tapTab("คู่หู");
  Usage a;a.primary=UsageWindow{11,300,1791097200};a.secondary=UsageWindow{22,10080,1791356400};data.claude={a,clockMs,false,false};
  Usage b;b.primary=UsageWindow{33,300,1791100800};b.secondary=UsageWindow{44,10080,1791442800};data.codex={b,clockMs,false,false};
  clockMs+=1000;capture(selected==Language::Thai?"duo-reset-both-th.ppm":"duo-reset-both-en.ppm");
  reset("Claude","11%","04/10 14:00");reset("Claude","22%","07/10 14:00");reset("Codex","33%","04/10 15:00");reset("Codex","44%","08/10 14:00");assertDuoVerticalSpacing();
  mode=CodexUsageMode::Weekly;clockMs+=1000;capture(selected==Language::Thai?"duo-reset-weekly-th.ppm":"duo-reset-weekly-en.ppm");
  reset("Codex","44%","08/10 14:00");
  const auto primaryReset=shapeText(std::string(selected==Language::Thai?"เริ่มใหม่ ":"Reset ")+"04/10 15:00");
  assert(!providerLabel("Codex",primaryReset.c_str()) && "Weekly-only must not display the 5h reset");
  data.codex.data->primary->windowMinutes=10080;clockMs+=1000;ui_tick();reset("Codex","33%","04/10 15:00");
  data.codex.data->primary->resetsAt.reset();clockMs+=1000;ui_tick();
  assert(providerLabel("Codex",shapeText(selected==Language::Thai?"เริ่มใหม่ --":"Reset --").c_str()) && "missing source reset stays unknown");
  data.codex.data->primary->windowMinutes=300;data.codex.data->secondary->windowMinutes=1440;clockMs+=1000;ui_tick();
  assert(providerLabel("Codex",shapeText(selected==Language::Thai?"เริ่มใหม่ --":"Reset --").c_str()));
  assert(!providerLabel("Codex",primaryReset.c_str()));
  mode=CodexUsageMode::Both;data.codex={b,clockMs,false,true};data.claude={a,clockMs,false,true};clockMs+=1000;ui_tick();
  assert(providerLabel("Claude",shapeText(selected==Language::Thai?"เก่า 0น.":"stale 0m").c_str()));
  assert(providerLabel("Codex",shapeText(selected==Language::Thai?"เก่า 0น.":"stale 0m").c_str()));
  reset("Claude","11%","04/10 14:00");reset("Codex","44%","08/10 14:00");
  clockMs=3600000ULL*100000;data.claude.receivedAtMs=0;data.codex.receivedAtMs=clockMs+1000;ui_tick();lv_obj_update_layout(lv_screen_active());
  assertTextFits(providerLabel("Claude",shapeText(selected==Language::Thai?"เก่า":"Stale").c_str()));
  assertTextFits(providerLabel("Codex",shapeText(selected==Language::Thai?"เก่า":"Stale").c_str()));
  clockMs=oldClock+1000;data.codex={};clockMs+=1000;ui_tick();
  assert(providerLabel("Codex",shapeText(selected==Language::Thai?"ไม่ทราบ":"unknown").c_str()));
  assert(providerLabel("Codex",shapeText(selected==Language::Thai?"เริ่มใหม่ --":"Reset --").c_str()));
 }
 data=original;language=oldLanguage;mode=oldMode;clockMs=oldClock+1000;ui_tick();
 std::cout<<"Duo resets: four distinct provider/window timestamps, TH/EN owned placement/width, selected Weekly primary/secondary, absent/ineligible reset, both stale sources, long/underflow ages and unknown source passed\n";
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
    for(auto* thai:{&ct_font_thai_12,&ct_font_thai_14,&ct_font_thai_18,&ct_font_ui_16,&ct_font_ui_18}){
        lv_font_t font=thai==&ct_font_ui_18?ct_font_ui_18:thai==&ct_font_ui_16?ct_font_ui_16:thai==&ct_font_thai_12?lv_font_montserrat_12:thai==&ct_font_thai_14?lv_font_montserrat_14:lv_font_montserrat_18;
        font.fallback=thai;font.line_height=thai->line_height;font.base_line=thai->base_line;
        for(unsigned cp=32;cp<=0xF765;++cp){
            if(!(cp<=126||cp==0xB0||(cp>=0xE01&&cp<=0xE3A)||(cp>=0xE3F&&cp<=0xE5B)||((cp>=0xF700&&cp<=0xF71B)||(cp>=0xF720&&cp<=0xF765))))continue;
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
        lv_obj_t* caption=nullptr;
        for(uint32_t k=0;k<lv_obj_get_child_count(navScreen);++k){auto* candidate=lv_obj_get_child(navScreen,k);if(lv_obj_get_y(candidate)==276 && lv_obj_get_x(candidate)==i*60)caption=findLabel(candidate,shaped.c_str());}
        assert(caption);
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
        const char* pageMarkers[]={"Claude","°C","หุ้น","ภาษา"};
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
    deskduo::Weather w;w.temperature=31;w.humidity=68;w.weatherCode=2;w.isDay=true;
    w.utcOffsetSeconds=25200;
    w.hourlyCount=4;
    for(int k=0;k<4;++k){w.hourlyTimes[k]=1791097200+k*3600;w.hourlyTemperatures[k]=32+k;w.hourlyRainProbability[k]=35+k;w.hourlyWeatherCodes[k]=k==1?63:2;w.hourlyIsDay[k]=true;}
    data.weatherLabel="กรุงเทพฯ";data.weather.data=w;data.weather.unknown=false;
    deskduo::Stocks stocks;stocks.count=5;
    for(int k=0;k<5;++k){stocks.items[k].symbol=k==0?"AAPL":k==1?"NVDA":"MSFT";stocks.items[k].price=k==4?1e300:225.40+k;stocks.items[k].changePercent=k%2?-1.24:2.81;stocks.items[k].label="หุ้นจำลอง";stocks.items[k].currency="USD";stocks.items[k].quotedAt=1791097200+k*60;}
    data.stocks.data=stocks;data.stocks.unknown=false;
    deskduo::language=deskduo::Language::English;
    clockMs=4000;capture("duo.ppm");
    assert(providerLabel("Claude","Reset 04/10 14:00") && "Claude reset must belong to its provider area");
    assert(providerLabel("Codex","Reset 04/10 14:00") && "Codex must show its own primary reset");
    assert(providerLabel("Claude","Weekly") && providerLabel("Claude","48%"));
    assert(providerLabel("Codex","Weekly") && providerLabel("Codex","21%"));
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
    auto* weeklyReset=providerLabel("Codex","Weekly");assert(weeklyReset);
    assertUsageColumnGap(providerLabel("Codex","Weekly"),providerLabel("Codex","21%"));
    auto* codexPrimaryBar=findBarBelow(weeklyReset);assert(codexPrimaryBar);
    lv_area_t primaryBarArea;lv_obj_get_coords(codexPrimaryBar,&primaryBarArea);
    assert(countVisible(countVisible,lv_screen_active(),&lv_bar_class)==3);
    auto* codexSecondaryBar=findBarBelow(codexPrimaryBar);assert(codexSecondaryBar && lv_obj_has_flag(codexSecondaryBar,LV_OBJ_FLAG_HIDDEN));
    unsigned hiddenSecondaryLabels=0;
    for(uint32_t i=0;i<lv_obj_get_child_count(lv_obj_get_parent(codexPrimaryBar));++i){
        auto* obj=lv_obj_get_child(lv_obj_get_parent(codexPrimaryBar),i);
        if(lv_obj_check_type(obj,&lv_label_class) && lv_obj_has_flag(obj,LV_OBJ_FLAG_HIDDEN) && lv_obj_get_y(obj)>lv_obj_get_y(codexPrimaryBar))++hiddenSecondaryLabels;
    }
    assert(hiddenSecondaryLabels==3 && "Weekly-only must hide both secondary columns and their reset");
    data.codex.data->primary=deskduo::UsageWindow{73.25,10080,4102444800};data.codex.data->secondary.reset();
    clockMs+=1000;capture("duo-weekly-primary.ppm");
    assert(findLabel(lv_screen_active(),"73.25%"));assert(findLabel(lv_screen_active(),"Weekly"));
    data.codex.data->primary->usedPercent=0;clockMs+=1000;deskduo::ui_tick();
    assert(findLabel(lv_screen_active(),"0%"));
    assert(!lv_obj_has_flag(codexPrimaryBar,LV_OBJ_FLAG_HIDDEN) && "eligible zero weekly usage must keep its primary bar visible");
    assert(lv_bar_get_value(codexPrimaryBar)==0);
    data.codex.data->primary->windowMinutes=300;clockMs+=1000;capture("duo-weekly-unavailable.ppm");
    assert(findLabel(lv_screen_active(),"Weekly"));assert(!findLabel(lv_screen_active(),"73.25%"));
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
    assert(findLabel(lv_screen_active(),"73.25%"));assert(findLabel(lv_screen_active(),"Weekly"));
    assert(!lv_obj_has_flag(codexPrimaryBar,LV_OBJ_FLAG_HIDDEN) && "eligible weekly data must restore the primary bar after unavailable data");
    deskduo::mode=deskduo::CodexUsageMode::Both;clockMs+=20;deskduo::ui_tick();
    assert(countVisible(countVisible,lv_screen_active(),&lv_bar_class)==4);
    assert(findLabel(lv_screen_active(),"window --"));
    data.codex.data->primary=deskduo::UsageWindow{58,90.5,4102444800};
    data.codex.data->secondary=deskduo::UsageWindow{21,2.5,4102531200};clockMs+=1000;deskduo::ui_tick();
    assert(findLabel(lv_screen_active(),"90.5m"));assert(providerLabel("Codex","2.5m") && providerLabel("Codex","21%"));
    data.codex.data=usage;
    data.claude.data->primary=deskduo::UsageWindow{99.99,90.5,4102444800};
    data.claude.data->secondary=deskduo::UsageWindow{73.25,2.5,4102531200};
    data.claude.stale=true;clockMs+=1000;capture("duo-stale-long-reset.ppm");
    assert(findLabel(lv_screen_active(),"90.5m"));
    assert(providerLabel("Claude","2.5m") && providerLabel("Claude","73.25%"));
    assertUsageColumnGap(providerLabel("Claude","90.5m"),providerLabel("Claude","99.99%"));
    assertUsageColumnGap(providerLabel("Claude","2.5m"),providerLabel("Claude","73.25%"));
    assert(findLabel(lv_screen_active(),"stale 0m"));
    assert(findLabel(lv_screen_active(),"stale 0m"));
    data.claude.data=usage;data.claude.stale=false;
    weatherChecks();
    setupLanguageChecks();
    stockChecks();
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
    assert(findLabel(screen,deskduo::shapeText("หุ้น").c_str()));
    clockMs+=5000;deskduo::ui_tick();
    assert(findLabel(screen,"Claude") && "automatic rotation must skip Setup");
    assert(!findLabel(screen,"hidden-new-token"));
    data.autoRotateSeconds=0;
    lv_mem_monitor_t before{},after{};lv_mem_monitor(&before);
    for(int i=0;i<40;++i) {tapTab("ตั้งค่า");tapTab("คู่หู");}
    lv_mem_monitor(&after);
    assert(after.free_size+512>=before.free_size && "repeated setup reveal/navigation must not leak LVGL heap");
    capture("navigation-cycled.ppm");
    scheduleSwitchChecks();
    autoBrightnessChecks();
    brightnessChecks();
    languageChecks();
    fidelityCaptures();
    duoSpacingChecks();
    duoResetChecks();
    std::cout<<"Setup token rotation, reflow, close/reopen checks passed; ";
    std::cout<<"Navigation/IP, fractional windows, rotation hold/skip checks passed; ";
    std::cout<<"LVGL heap after 40 reveal/close cycles: "<<after.total_size-after.free_size<<" bytes; ";
    std::cout<<"Resolved glyph checks: "<<checks<<"; simulated 240x320 screenshots rendered by actual ui.cpp\n";
}
