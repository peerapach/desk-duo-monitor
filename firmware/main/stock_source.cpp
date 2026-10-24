#include "stock_source.hpp"
#include <set>
namespace deskduo {
std::string stockChartUrl(const std::string& symbol, StockChartRange range) {
    if (!validStockSymbol(symbol) || (range != StockChartRange::Day && range != StockChartRange::FiveDays)) return {};
    return "https://query1.finance.yahoo.com/v8/finance/chart/" + symbol +
        (range == StockChartRange::Day ? "?range=1d&interval=5m" : "?range=5d&interval=30m") + "&includePrePost=false";
}
ParseResult<Stocks> pollYahooStocks(const SourceConfig& source, const Stocks* previous, int64_t nowEpoch, const StockFetcher& fetcher) {
    if (source.provider != StockProvider::Yahoo || source.refreshSeconds < 300 || source.refreshSeconds > 86400 ||
        source.watchCount > source.watchlist.size() || (source.chartRange != StockChartRange::Day && source.chartRange != StockChartRange::FiveDays)) return {std::nullopt, "selection"};
    std::set<std::string> symbols;
    for (std::size_t i = 0; i < source.watchCount; ++i) {
        if (!validStockSymbol(source.watchlist[i].symbol) || !symbols.insert(source.watchlist[i].symbol).second) return {std::nullopt, "selection"};
    }
    Stocks result;
    result.generatedAt = nowEpoch;
    std::size_t successes = 0;
    for (std::size_t i = 0; i < source.watchCount; ++i) {
        const auto& selection = source.watchlist[i];
        if (!selection.enabled) continue;
        // Construct a clean request: legacy URL, credentials and field mappings
        // never enter the provider request, including across provider changes.
        SourceConfig request;
        request.url = stockChartUrl(selection.symbol, source.chartRange);
        request.refreshSeconds = source.refreshSeconds;
        const auto body = fetcher(request);
        const auto parsed = body ? parseYahooChart(*body, selection.symbol, source.chartRange) : ParseResult<Stock>{std::nullopt, "fetch"};
        auto& item = result.items[result.count++];
        if (parsed.value) {
            item = *parsed.value;
            ++successes;
            continue;
        }
        result.partialFailure = true;
        item.symbol = selection.symbol;
        item.label = selection.symbol;
        item.chartRange = source.chartRange;
        if (previous) for (std::size_t old = 0; old < previous->count && old < previous->items.size(); ++old) {
            const auto& candidate = previous->items[old];
            if (candidate.symbol == selection.symbol && candidate.chartRange == source.chartRange) { item = candidate; break; }
        }
        item.stale = true;
    }
    // A failed poll must not renew receipt time or publish a fresh success.
    // Runtime markFailed preserves the entire previous snapshot in this case.
    if (result.count && !successes) return {std::nullopt, "fetch"};
    return {std::move(result), {}};
}
}
