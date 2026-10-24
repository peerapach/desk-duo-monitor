#pragma once
#include "core.hpp"
#include <functional>
namespace deskduo {
using StockFetcher = std::function<std::optional<std::string>(const SourceConfig&)>;
std::string stockChartUrl(const std::string& symbol, StockChartRange range);
ParseResult<Stocks> pollYahooStocks(const SourceConfig& source, const Stocks* previous,
                                   int64_t nowEpoch, const StockFetcher& fetcher);
}
