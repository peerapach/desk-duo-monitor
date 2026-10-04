#pragma once
#include "core.hpp"
#include <algorithm>
namespace deskduo {
struct TransportPolicy { bool secure; bool mayConnect; };
inline TransportPolicy transportPolicy(const std::string& url, int64_t epochSeconds) {
    // Match the core's ASCII case-insensitive scheme acceptance for every hop.
    std::string scheme = url.substr(0, url.find("://"));
    for (char& character : scheme)
        if (character >= 'A' && character <= 'Z') character += 'a' - 'A';
    const bool secure = scheme == "https";
    // Fail closed even if a future caller supplies a URL outside core validation.
    return {secure, redirectTargetAllowed(url, url) && (!secure || epochSeconds >= 1700000000)};
}
inline std::string resolveRedirectTarget(const std::string& from, const std::string& location) {
    if (location.rfind("/", 0) == 0 && location.rfind("//", 0) != 0) {
        const auto schemeEnd = from.find("://");
        if (schemeEnd == std::string::npos) return {};
        // An authority ends at a path, query or fragment, not a slash in a query.
        const auto end = from.find_first_of("/?#", schemeEnd + 3);
        return from.substr(0, end) + location;
    }
    return location;
}
struct FetchPolicy {
    std::string url;
    uint64_t started;
    unsigned redirects = 0;
    std::size_t bodyBytes = 0;
    bool sendHeaders = true;
    FetchPolicy(std::string source, uint64_t now) : url(std::move(source)), started(now) {}
    int remaining(uint64_t now) const { return now - started >= 10000 ? 0 : 10000 - static_cast<int>(now - started); }
    bool append(std::size_t bytes) {
        if (bytes > 16384 - bodyBytes) return false;
        bodyBytes += bytes;
        return true;
    }
    bool redirect(const std::string& location) {
        const auto target = resolveRedirectTarget(url, location);
        if (redirects >= 4 || !redirectTargetAllowed(url, target)) return false;
        sendHeaders = sendHeaders && redirectHeadersAllowed(url, target);
        url = target;
        bodyBytes = 0;
        ++redirects;
        return true;
    }
};
std::string station_address();
void network_init();
void network_task(void*);
}
