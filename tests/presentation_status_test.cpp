#include "../firmware/main/presentation_status.hpp"
#include "cJSON.h"
#include <cassert>
#include <cmath>
#include <iostream>
#include <limits>
using namespace deskduo;
int main() {
    Snapshot s;
    auto json=dashboard_json(s);
    auto* root=cJSON_Parse(json.c_str());assert(root && cJSON_GetArraySize(root)==3);cJSON_Delete(root);
    assert(json.find(R"("chartRange":"1d")")!=std::string::npos);
    s.stockChartRange=StockChartRange::FiveDays;
    assert(dashboard_json(s).find(R"("chartRange":"5d")")!=std::string::npos);
    Usage u;u.provider="secret";u.updatedAt=1790000000;u.primary=UsageWindow{42.5,300.5,1790000010};
    u.secondary=UsageWindow{12,std::nullopt,std::nullopt};s.claude={u,10,false,true};
    Stocks stocks;stocks.count=2;stocks.items[0].symbol="AAPL";stocks.items[0].currency="USD";
    stocks.items[0].label="secret-label";stocks.items[0].price=249.5;stocks.items[0].changePercent=-1.25;
    stocks.items[0].quotedAt=1790000020;stocks.items[0].stale=true;
    auto history=std::make_shared<std::array<StockPoint,32>>();
    for(size_t i=0;i<32;++i)(*history)[i].close=i==1?std::nullopt:std::optional<double>(100+i);
    stocks.items[0].history=history;stocks.items[0].historyCount=999;stocks.items[1].symbol="DO-NOT-EXPOSE";
    s.stocks={stocks,10,false,false};s.weatherLabel="secret-weather";
    json=dashboard_json(s);root=cJSON_Parse(json.c_str());assert(root);
    auto* claude=cJSON_GetObjectItem(root,"claude");assert(cJSON_IsTrue(cJSON_GetObjectItem(claude,"hasData")));
    assert(cJSON_IsTrue(cJSON_GetObjectItem(claude,"stale")));
    auto* primary=cJSON_GetObjectItem(claude,"primary");assert(cJSON_GetObjectItem(primary,"usedPercent")->valuedouble==42.5);
    assert(cJSON_GetObjectItem(primary,"windowMinutes")->valuedouble==300.5);
    assert(cJSON_GetObjectItem(primary,"resetsAt")->valuedouble==1790000010);
    auto* stock=cJSON_GetObjectItem(root,"stock");assert(cJSON_IsTrue(cJSON_GetObjectItem(stock,"stale")));
    assert(cJSON_GetObjectItem(stock,"price")->valuedouble==249.5);
    assert(json.find(R"("chartRange":"1d")")!=std::string::npos); // Actual day series, despite saved five-day range.
    auto* points=cJSON_GetObjectItem(stock,"history");assert(cJSON_GetArraySize(points)==32);
    assert(cJSON_IsNull(cJSON_GetArrayItem(points,1)) && cJSON_GetArrayItem(points,31)->valuedouble==131);
    cJSON_Delete(root);assert(json.size()<4096 && json.find("secret")==std::string::npos && json.find("DO-NOT")==std::string::npos);
    s.stocks.data->items[0].chartRange=StockChartRange::FiveDays;
    assert(dashboard_json(s).find(R"("chartRange":"5d")")!=std::string::npos);
    s.stocks.data->items[0].symbol=std::string(32,'L');
    assert(dashboard_json(s).find(std::string(32,'L'))!=std::string::npos);
    s.claude.unknown=true;s.stocks.unknown=true;
    json=dashboard_json(s);assert(json.find("249.5")==std::string::npos && json.find("42.5")==std::string::npos);
    assert(json.find(R"("chartRange":"5d")")!=std::string::npos);
    s.stocks.unknown=false;s.stocks.data->items[0].price=std::numeric_limits<double>::infinity();
    s.stocks.data->items[0].symbol=std::string(100000,'x');s.stocks.data->items[0].currency=std::string(100000,'x');
    json=dashboard_json(s);assert(json.size()<4096 && json.find("inf")==std::string::npos);
    root=cJSON_Parse(json.c_str());assert(root);cJSON_Delete(root);
    std::cout<<"Dashboard actual windows, privacy, unknown/stale, finite and history bounds passed\n";
}
