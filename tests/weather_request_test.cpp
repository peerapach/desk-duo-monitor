#include "../firmware/main/weather_request.hpp"
#include <cassert>
#include <iostream>
using namespace deskduo;
int main() {
    SourceConfig source;
    source.url="https://api.open-meteo.com/v1/forecast?latitude=13.7563&longitude=100.5018&current=temperature_2m,relative_humidity_2m,weather_code&hourly=temperature_2m,precipitation_probability&timezone=Asia%2FBangkok&key=secret";
    const auto original=source.url;
    const auto expected="https://api.open-meteo.com/v1/forecast?latitude=13.7563&longitude=100.5018&current=temperature_2m,relative_humidity_2m,weather_code,is_day&hourly=temperature_2m,precipitation_probability,weather_code,is_day&timezone=Asia%2FBangkok&key=secret";
    assert(weather_request_url(source)==expected && source.url==original);
    source.url=expected;assert(weather_request_url(source)==expected);
    // Standard URLSearchParams output from the friendly location form (legacy fields lacked day/art fields).
    const std::string generated="https://api.open-meteo.com/v1/forecast?latitude=13.7563&longitude=100.5018&current=temperature_2m%2Crelative_humidity_2m%2Cweather_code&hourly=temperature_2m%2Cprecipitation_probability&timezone=auto";
    const std::string enriched="https://api.open-meteo.com/v1/forecast?latitude=13.7563&longitude=100.5018&current=temperature_2m%2Crelative_humidity_2m%2Cweather_code%2Cis_day&hourly=temperature_2m%2Cprecipitation_probability%2Cweather_code%2Cis_day&timezone=auto";
    source.url=generated;assert(weather_request_url(source)==enriched && source.url==generated);
    source.url=enriched;assert(weather_request_url(source)==enriched);
    source.url="https://api.open-meteo.com/v1/forecast?current=temperature_2m%2crelative_humidity_2m,weather_code&hourly=temperature_2m,precipitation_probability%2cweather_code&key=keep%2Csecret%26data";
    assert(weather_request_url(source)=="https://api.open-meteo.com/v1/forecast?current=temperature_2m%2crelative_humidity_2m,weather_code%2cis_day&hourly=temperature_2m,precipitation_probability%2cweather_code,is_day&key=keep%2Csecret%26data");
    source.url="https://api.open-meteo.com/v1/forecast?current=temperature_2m%2Cis_day&hourly=temperature_2m%2cweather_code%2cis_day";
    assert(weather_request_url(source)==source.url);
    source.url="https://api.open-meteo.com/v1/forecast?current=temperature_2m%2Cweather_code";
    assert(weather_request_url(source)=="https://api.open-meteo.com/v1/forecast?current=temperature_2m%2Cweather_code%2Cis_day&hourly=weather_code,is_day");
    for(auto value:{"temperature_2m%", "temperature_2m%2", "temperature_2m%2Gweather_code", "temperature_2m%20weather_code", "temperature_2m%252Cweather_code", "temperature_2m%2C%2Cweather_code"}) {
        source.url=std::string("https://api.open-meteo.com/v1/forecast?current=")+value;
        assert(weather_request_url(source)==source.url);
    }
    source.url="https://custom.test/v1/forecast?current=temperature_2m%2Cweather_code";
    assert(weather_request_url(source)==source.url);
    source.url=generated+"&token="+std::string(790,'s');
    assert(source.url.size()<1024 && weather_request_url(source)==source.url);
    for(auto url:{"http://api.open-meteo.com/v1/forecast?current=temperature_2m", "https://evil.test/v1/forecast?current=temperature_2m", "https://api.open-meteo.com/v1/forecast?current=temperature_2m&current=weather_code", "https://api.open-meteo.com/v1/forecast?current=temperature_2m&%68ourly=temperature_2m", "https://api.open-meteo.com/v1/forecast?current_weather=true", "https://api.open-meteo.com/v1/forecast?current=temperature_2m&", "https://api.open-meteo.com/v1/forecast?current=temperature_2m#fragment", "https://api.open-meteo.com/v1/forecast?current=temperature_2m&hourly=temperature_2m&key=a&key=b"}) {
        source.url=url;assert(weather_request_url(source)==url);
    }
    source.url=original;source.fields["isDay"]="custom.day";source.fields["hourlyIsDay"]="custom.hour";source.fields["hourlyWeatherCodes"]="custom.code";
    assert(weather_request_url(source)==original);
    source.fields.clear();source.url="https://api.open-meteo.com/v1/forecast?current=temperature_2m&hourly=temperature_2m&token="+std::string(920,'x');
    assert(source.url.size()<1024 && weather_request_url(source)==source.url);
    source.fields.clear();source.url="https://api.open-meteo.com/v1/forecast?current=temperature_2m&token="+std::string(980,'x');
    assert(weather_request_url(source)==source.url);
    source.url="https://api.open-meteo.com/v1/forecast?current=temperature_2m";
    assert(weather_request_url(source)=="https://api.open-meteo.com/v1/forecast?current=temperature_2m,is_day&hourly=weather_code,is_day");
    std::cout<<"Conservative Open-Meteo request enrichment and unchanged custom/secret/query data passed\n";
}
