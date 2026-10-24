#include "presentation_status.hpp"
#include <algorithm>
#include <cmath>
#include <cstdio>

namespace deskduo {
namespace {
const char* boolean(bool value) { return value ? "true" : "false"; }
std::string number(double value) {
    if (!std::isfinite(value)) return "null";
    char buffer[32]; std::snprintf(buffer, sizeof(buffer), "%.17g", value); return buffer;
}
std::string number(const std::optional<double>& value) { return value ? number(*value) : "null"; }
std::string timestamp(const std::optional<int64_t>& value) { return value && *value > 0 ? std::to_string(*value) : "null"; }
std::string quoted(const std::string& value, std::size_t cap) {
    if (value.size() > cap) return "\"\"";
    std::string out="\"";
    for (unsigned char ch : value) {
        if (ch=='"' || ch=='\\') { out+='\\'; out+=ch; }
        else if (ch<32) { char escaped[7]; std::snprintf(escaped,sizeof(escaped),"\\u%04x",ch); out+=escaped; }
        else out+=ch;
    }
    return out+'"';
}
std::string window(const std::optional<UsageWindow>& value) {
    if (!value) return "null";
    return "{\"usedPercent\":"+number(value->usedPercent)+",\"windowMinutes\":"+number(value->windowMinutes)+
           ",\"resetsAt\":"+timestamp(value->resetsAt)+"}";
}
std::string usage(const SourceSnapshot<Usage>& source) {
    const bool present=source.data.has_value() && !source.unknown;
    return std::string("{\"hasData\":")+boolean(present)+",\"stale\":"+boolean(present && source.stale)+
        ",\"updatedAt\":"+(present ? timestamp(source.data->updatedAt) : "null")+
        ",\"primary\":"+(present ? window(source.data->primary) : "null")+
        ",\"secondary\":"+(present ? window(source.data->secondary) : "null")+"}";
}
std::string stock(const SourceSnapshot<Stocks>& source, StockChartRange fallbackRange) {
    const Stock* item=nullptr;
    if (source.data && !source.unknown) {
        for (std::size_t i=0; i<std::min(source.data->count,source.data->items.size()); ++i) {
            if (!source.data->items[i].symbol.empty()) { item=&source.data->items[i]; break; }
        }
    }
    std::string out=std::string("{\"hasData\":")+boolean(item)+",\"stale\":"+boolean(item && (source.stale || item->stale))+
        ",\"symbol\":"+(item ? quoted(item->symbol,32) : "\"\"")+
        ",\"currency\":"+(item ? quoted(item->currency,8) : "\"\"")+
        ",\"price\":"+(item ? number(item->price) : "null")+
        ",\"changePercent\":"+(item ? number(item->changePercent) : "null")+
        ",\"quotedAt\":"+(item ? timestamp(item->quotedAt) : "null")+",\"chartRange\":\""+((item ? item->chartRange : fallbackRange)==StockChartRange::FiveDays ? "5d" : "1d")+"\",\"history\":[";
    if (item && item->history) {
        for (std::size_t i=0;i<std::min(item->historyCount,item->history->size());++i) {
            if(i) out+=',';
            out+=number((*item->history)[i].close);
        }
    }
    return out+"]}";
}
}
std::string dashboard_json(const Snapshot& snapshot) {
    return "{\"claude\":"+usage(snapshot.claude)+",\"codex\":"+usage(snapshot.codex)+",\"stock\":"+stock(snapshot.stocks,snapshot.stockChartRange)+"}";
}
}
