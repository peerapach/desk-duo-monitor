#pragma once
#include "core.hpp"
#include <algorithm>
namespace deskduo {
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
    bool redirect(const std::string& target) {
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
