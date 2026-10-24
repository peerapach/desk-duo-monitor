#include "../firmware/main/stock_source.hpp"
#include "../firmware/main/runtime.hpp"
#include <iostream>
#include <stdexcept>
#include <cmath>
using namespace deskduo;
#define CHECK(x) do { ++checks; if (!(x)) throw std::runtime_error(std::string("line ") + std::to_string(__LINE__) + ": " #x); } while(0)
int checks = 0;
std::string fixture(std::string timestamps = "1000,1100,1200", std::string closes = "100,null,110", std::string symbol = "AAPL", std::string extra = "\"chartPreviousClose\":100,") {
    return "{\"chart\":{\"error\":null,\"result\":[{\"meta\":{\"symbol\":\"" + symbol + "\",\"currency\":\"USD\",\"regularMarketPrice\":110," + extra + "\"regularMarketTime\":1200},\"timestamp\":[" + timestamps + "],\"indicators\":{\"quote\":[{\"close\":[" + closes + "]}]}}]}}";
}
void parser() {
    static_assert(sizeof(Snapshot) < 2048, "snapshots must fit the ESP task stack; history is shared immutable storage");
    auto stock = parseYahooChart(fixture(), "AAPL", StockChartRange::Day);
    CHECK(stock.value); CHECK(stock.value->price == 110); CHECK(stock.value->changePercent && std::abs(*stock.value->changePercent - 10) < 1e-9);
    CHECK(stock.value->historyCount == 3); CHECK(!(*stock.value->history)[1].close); CHECK((*stock.value->history)[2].at == 1200);
    const auto copied = *stock.value;
    CHECK(copied.history == stock.value->history);
    CHECK(!parseYahooChart(fixture(), "MSFT", StockChartRange::Day).value);
    CHECK(!parseYahooChart(fixture("1000,999,1200"), "AAPL", StockChartRange::Day).value);
    CHECK(!parseYahooChart(fixture("1000,1100"), "AAPL", StockChartRange::Day).value);
    for (const auto& bad : {"-1,null,110", "1e400,null,110", "true,null,110"}) CHECK(!parseYahooChart(fixture("1000,1100,1200",bad), "AAPL", StockChartRange::Day).value);
    for (const auto& bad : {"-1,1100,1200", "1.5,1100,1200", "null,1100,1200", "1000,1100,253402300800"}) CHECK(!parseYahooChart(fixture(bad), "AAPL", StockChartRange::Day).value);
    auto empty = parseYahooChart(fixture("", ""), "AAPL", StockChartRange::Day);
    CHECK(empty.value && empty.value->historyCount == 0);
    auto gaps = parseYahooChart(fixture("1000,1100,1200", "null,null,null"), "AAPL", StockChartRange::FiveDays);
    CHECK(gaps.value && !gaps.value->changePercent && !(*gaps.value->history)[0].close);
    auto thai = fixture("1000,1100,1200", "100,null,110", "PTT.BK");
    thai.replace(thai.find("USD"), 3, "THB"); CHECK(parseYahooChart(thai,"PTT.BK",StockChartRange::Day).value);
    CHECK(!parseYahooChart("{", "AAPL", StockChartRange::Day).value);
    CHECK(!parseYahooChart(fixture()+"x", "AAPL", StockChartRange::Day).value);
    CHECK(!parseYahooChart(std::string(16385,' '), "AAPL", StockChartRange::Day).value);
    CHECK(!parseYahooChart(R"({"chart":{"error":{"code":"429"},"result":null}})", "AAPL", StockChartRange::Day).value);
    auto noReference = parseYahooChart(fixture("1000,1100,1200", "100,null,110", "AAPL", ""), "AAPL", StockChartRange::Day);
    CHECK(noReference.value && !noReference.value->changePercent);
    CHECK(!parseYahooChart(fixture("1000,1100,1200", "100,null,110", "AAPL", "\"chartPreviousClose\":0,"), "AAPL", StockChartRange::Day).value);
    auto five = parseYahooChart(fixture("1000,1100,1200", "null,100,110"), "AAPL", StockChartRange::FiveDays);
    CHECK(five.value && five.value->changePercent && std::abs(*five.value->changePercent - 10) < 1e-9);
    auto flat = parseYahooChart(fixture("1000,1100,1200", "110,110,110"), "AAPL", StockChartRange::FiveDays);
    CHECK(flat.value && flat.value->changePercent == 0);
    std::string times, prices;
    for (int i=0;i<100;++i) { if(i) { times += ','; prices += ','; } times += std::to_string(1000+i); prices += i==1 ? "null" : "100"; }
    auto sampled = parseYahooChart(fixture(times,prices), "AAPL", StockChartRange::FiveDays);
    CHECK(sampled.value && sampled.value->historyCount <= 32); CHECK((*sampled.value->history)[0].at == 1000);
    bool gap=false; for(std::size_t i=0;i<sampled.value->historyCount;++i) gap |= !(*sampled.value->history)[i].close;
    CHECK(gap); CHECK((*sampled.value->history)[sampled.value->historyCount-1].at == 1099);
    auto currency=fixture(); currency.replace(currency.find("USD"),3,"THB"); CHECK(!parseYahooChart(currency,"AAPL",StockChartRange::Day).value);
}
void config() {
    auto legacy = parseStoredConfig(R"({"stocks":{"url":"https://example.test/stocks","refreshSeconds":30}})");
    CHECK(legacy.value && legacy.value->stocks.provider == StockProvider::Json);
    auto config = applyConfigPatch(*legacy.value, R"({"stocks":{"provider":"yahoo","refreshSeconds":300,"chartRange":"5d","watchlist":[{"symbol":"ptt.bk","enabled":false},{"symbol":"AAPL"}]}})");
    CHECK(config.value); CHECK(config.value->stocks.watchCount==2); CHECK(config.value->stocks.watchlist[0].symbol=="PTT.BK");
    CHECK(completeSerialization(*config.value, serializeConfig(*config.value)));
    CHECK(!completeSerialization(*config.value, serializeConfig(*legacy.value)));
    auto sparse = applyConfigPatch(*config.value,R"({"autoRotateSeconds":15})"); CHECK(sparse.value); CHECK(completeSerialization(*config.value,serializeConfig(*sparse.value))==false); CHECK(sparse.value->stocks.watchlist==config.value->stocks.watchlist);
    for(const auto& bad:{R"({"stocks":{"refreshSeconds":299}})",R"({"stocks":{"watchlist":[{"symbol":"AAPL"},{"symbol":"aapl"}]}})",R"({"stocks":{"watchlist":[{"symbol":"AAPL/evil"}]}})",R"({"stocks":{"watchlist":[{"symbol":"ABCDEFGHIJKLMNOPQRSTU"}]}})",R"({"weather":{"provider":"json"}})"}) CHECK(!applyConfigPatch(*config.value,bad).value);
    CHECK(!applyConfigPatch(*config.value, R"({"stocks":{"watchlist":[{"symbol":"A"},{"symbol":"B"},{"symbol":"C"},{"symbol":"D"},{"symbol":"E"},{"symbol":"F"}]}})").value);
    CHECK(!applyConfigPatch(*config.value, R"({"stocks":{"watchlist":[{"symbol":"AAPL","enabled":1}]}})").value);
    CHECK(!applyConfigPatch(*config.value, R"({"stocks":{"provider":"other"}})").value);
    CHECK(!applyConfigPatch(*config.value, R"({"stocks":{"chartRange":"1mo"}})").value);
    Model model; model.applyConfig(*config.value); auto old=model.generation(); Stocks stocks; model.acceptStocks(stocks,10,old);
    auto next=*config.value; next.stocks.chartRange=StockChartRange::Day; model.applyConfig(next); CHECK(model.generation()!=old); model.acceptStocks(stocks,20,old); CHECK(!model.snapshot(20).stocks.data); CHECK(model.snapshot(20).stocksConfigured);
    old=model.generation(); next.stocks.watchlist[0].enabled=true; model.applyConfig(next); CHECK(model.generation()!=old);
    old=model.generation(); next.stocks.watchlist[0].symbol="MSFT"; model.applyConfig(next); CHECK(model.generation()!=old);
    old=model.generation(); next.stocks.provider=StockProvider::Json; model.applyConfig(next); CHECK(model.generation()!=old);
}
void poll() {
    auto source=defaultConfig().stocks; source.provider=StockProvider::Yahoo; source.headers["Authorization"]="secret"; source.watchCount=3;
    source.watchlist[0]={"AAPL",true};source.watchlist[1]={"MSFT",false};source.watchlist[2]={"GOOG",true};
    int calls=0;
    auto result=pollYahooStocks(source,nullptr,1300,[&](const SourceConfig& request)->std::optional<std::string>{CHECK(request.headers.empty());CHECK(request.url==stockChartUrl(calls++==0?"AAPL":"GOOG",StockChartRange::Day));return fixture("1000,1100,1200","100,null,110",calls==1?"AAPL":"GOOG");});
    CHECK(result.value && result.value->count==2 && calls==2); CHECK(!result.value->partialFailure);
    auto previous=*result.value; calls=0;
    auto partial=pollYahooStocks(source,&previous,1400,[&](const SourceConfig&)->std::optional<std::string>{if(calls++==0) return std::nullopt; return fixture("1000,1100,1200","100,null,110","GOOG");});
    CHECK(partial.value && partial.value->partialFailure); CHECK(partial.value->items[0].stale && partial.value->items[0].price==110); CHECK(!partial.value->items[1].stale);
    auto failure=pollYahooStocks(source,&previous,1500,[](const SourceConfig&)->std::optional<std::string>{return std::nullopt;}); CHECK(!failure.value);
    source.chartRange=StockChartRange::FiveDays;
    auto wrongRange=pollYahooStocks(source,&previous,1600,[&](const SourceConfig&)->std::optional<std::string>{return calls++%2?std::optional<std::string>{fixture("1000,1100,1200","100,null,110","GOOG")}:std::nullopt;});
    CHECK(wrongRange.value && !wrongRange.value->items[0].price);
    for(auto& entry:source.watchlist) entry.enabled=false;
    calls=0;auto disabled=pollYahooStocks(source,nullptr,1600,[&](const SourceConfig&)->std::optional<std::string>{++calls;return {};});CHECK(disabled.value && disabled.value->count==0 && calls==0);
    CHECK(stockChartUrl("AAPL",StockChartRange::Day)=="https://query1.finance.yahoo.com/v8/finance/chart/AAPL?range=1d&interval=5m&includePrePost=false");
    CHECK(stockChartUrl("PTT.BK",StockChartRange::FiveDays)=="https://query1.finance.yahoo.com/v8/finance/chart/PTT.BK?range=5d&interval=30m&includePrePost=false");CHECK(stockChartUrl("AAPL?x=1",StockChartRange::Day).empty());
}
int main(){try{config();parser();poll();std::cout<<"stock source checks passed: "<<checks<<"\n";return 0;}catch(const std::exception& e){std::cerr<<e.what()<<"\n";return 1;}}
