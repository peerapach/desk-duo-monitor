#pragma once
#include <algorithm>
#include <cctype>
#include <string>
namespace deskduo {
inline bool constantEqual(const std::string& a, const std::string& b) {
    volatile unsigned difference = static_cast<unsigned>(a.size() ^ b.size());
    for (std::size_t i = 0; i < std::max(a.size(), b.size()); ++i)
        difference |= (i < a.size() ? static_cast<unsigned char>(a[i]) : 0) ^
                      (i < b.size() ? static_cast<unsigned char>(b[i]) : 0);
    return difference == 0;
}
inline std::string basicCredential(const std::string& password) {
    const std::string raw = "admin:" + password;
    const char* alphabet = "ABCDEFGHIJKLMNOPQRSTUVWXYZabcdefghijklmnopqrstuvwxyz0123456789+/";
    std::string out = "Basic ";
    for (std::size_t i = 0; i < raw.size(); i += 3) {
        unsigned value = static_cast<unsigned char>(raw[i]) << 16;
        if (i + 1 < raw.size()) value |= static_cast<unsigned char>(raw[i + 1]) << 8;
        if (i + 2 < raw.size()) value |= static_cast<unsigned char>(raw[i + 2]);
        out += alphabet[(value >> 18) & 63]; out += alphabet[(value >> 12) & 63];
        out += i + 1 < raw.size() ? alphabet[(value >> 6) & 63] : '=';
        out += i + 2 < raw.size() ? alphabet[value & 63] : '=';
    }
    return out;
}
inline bool authorized(const std::string& header, bool usage, const std::string& setup, const std::string& token) {
    if (usage && token.empty()) return false;
    return constantEqual(header, usage ? "Bearer " + token : basicCredential(setup));
}
// Zero means accepted; otherwise an HTTP status. No form POSTs / permissive CORS.
inline int bodyPolicy(std::size_t length, std::string type, std::size_t limit) {
    if (length > limit) return 413;
    if (length == 0) return 400;
    type = type.substr(0, type.find(';'));
    while (!type.empty() && type.back() == ' ') type.pop_back();
    std::transform(type.begin(), type.end(), type.begin(), [](unsigned char c) { return std::tolower(c); });
    return type == "application/json" ? 0 : 415;
}
void portal_init();
}
