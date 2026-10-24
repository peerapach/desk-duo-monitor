#include "weather_request.hpp"
#include <set>
#include <string_view>
#include <vector>
namespace deskduo {
namespace {
bool defaultField(const SourceConfig& source, const char* key, const char* target) {
    auto it=source.fields.find(key);
    return it==source.fields.end() || it->second==target;
}
// Decode only list separators for token comparison. Keep the original value for
// sending, including mixed separator spelling and every unrelated query byte.
bool fieldList(const std::string& value, std::string& tokens, std::string& separator) {
    tokens.clear(); separator.clear();
    for (std::size_t i=0; i<value.size(); ++i) {
        char ch=value[i];
        if(ch=='%') {
            if(i+2>=value.size() || value[i+1]!='2' || (value[i+2]!='C' && value[i+2]!='c')) return false;
            if(separator.empty()) separator=value.substr(i,3);
            tokens+=','; i+=2;
        } else {
            if(ch==',' && separator.empty()) separator=",";
            tokens+=ch;
        }
    }
    if(separator.empty()) separator=",";
    return tokens.find_first_not_of("abcdefghijklmnopqrstuvwxyz0123456789_,")==std::string::npos &&
        (tokens.empty() || (tokens.front()!=',' && tokens.back()!=',' && tokens.find(",,")==std::string::npos));
}
bool addField(std::string& value, const char* field) {
    std::string tokens, separator;
    if(!fieldList(value,tokens,separator)) return false;
    std::size_t pos=0;
    while(pos<tokens.size()) {
        auto end=tokens.find(',',pos);
        if(tokens.substr(pos,end==std::string::npos ? end : end-pos)==field) return false;
        if(end==std::string::npos) break;
        pos=end+1;
    }
    if(!value.empty()) value+=separator;
    value+=field; return true;
}
}
std::string weather_request_url(const SourceConfig& source) {
    const auto& url=source.url;
    constexpr std::string_view prefix="https://api.open-meteo.com/v1/forecast?";
    if(url.size()>1024 || url.compare(0,prefix.size(),prefix)!=0 || url.find('#')!=std::string::npos || (!url.empty() && url.back()=='&')) return url;
    std::vector<std::pair<std::string,std::string>> params;
    std::set<std::string> names;
    std::size_t pos=prefix.size(), current=std::string::npos, hourly=std::string::npos;
    while(pos<url.size()) {
        auto end=url.find('&',pos); if(end==std::string::npos) end=url.size();
        auto eq=url.find('=',pos);
        if(eq==std::string::npos || eq>=end) return url;
        auto name=url.substr(pos,eq-pos);
        // Unknown encodings or duplicate names can alias current/hourly; leave intact.
        if(name.empty() || name.find('%')!=std::string::npos || name.find('+')!=std::string::npos || !names.insert(name).second) return url;
        if(name=="current") current=params.size();
        if(name=="hourly") hourly=params.size();
        params.emplace_back(std::move(name),url.substr(eq+1,end-eq-1));
        pos=end+1;
    }
    if(current==std::string::npos || params[current].second.empty()) return url;
    std::string tokens, separator;
    if(!fieldList(params[current].second,tokens,separator) ||
       (hourly!=std::string::npos && !fieldList(params[hourly].second,tokens,separator))) return url;
    bool changed=false;
    if(defaultField(source,"isDay","current.is_day")) changed|=addField(params[current].second,"is_day");
    std::string hours=hourly==std::string::npos ? "" : params[hourly].second;
    bool hoursChanged=false;
    if(defaultField(source,"hourlyWeatherCodes","hourly.weather_code")) hoursChanged|=addField(hours,"weather_code");
    if(defaultField(source,"hourlyIsDay","hourly.is_day")) hoursChanged|=addField(hours,"is_day");
    if(hoursChanged) {
        if(hourly==std::string::npos) params.emplace_back("hourly",hours);
        else params[hourly].second=hours;
        changed=true;
    }
    if(!changed) return url;
    std::string result(prefix);
    for(std::size_t i=0;i<params.size();++i) {
        if(i) result+='&';
        result+=params[i].first+'='+params[i].second;
    }
    return result.size()<=1024 ? result : url;
}
}
