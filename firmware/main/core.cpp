#include "core.hpp"

#include <cJSON.h>

#include <algorithm>
#include <cctype>
#include <cmath>
#include <cstdio>
#include <cstring>
#include <limits>
#include <memory>
#include <set>
#include <string_view>
#include <vector>

namespace deskduo {
namespace {

constexpr std::size_t kUsageBodyLimit = 4096;
constexpr std::size_t kSourceBodyLimit = 16384;
constexpr std::size_t kConfigBodyLimit = 8192;
constexpr std::size_t kStoredConfigLimit = 32768;
// Every accepted persisted string is bounded below; 4x covers quote/UTF-8 JSON escaping.
// 2048 bytes covers all fixed keys, separators, booleans, numbers, and object punctuation.
constexpr std::size_t kMaximumPersistedStringBytes = 288 + 2 * 1024 + 12 * (64 + 256) + 48 + 8 * (21 + 128);
constexpr std::size_t kMaximumSerializedConfigBytes = 4 * kMaximumPersistedStringBytes + 2048;
static_assert(kMaximumSerializedConfigBytes <= kStoredConfigLimit, "stored config cap must fit every valid config");
constexpr std::size_t kMaximumJsonDepth = 16;
constexpr std::size_t kMaximumUrlLength = 1024;
constexpr std::size_t kMaximumHeaderCount = 6;
constexpr std::size_t kMaximumHeaderNameLength = 64;
constexpr std::size_t kMaximumHeaderValueLength = 256;
constexpr std::size_t kMaximumMappingLength = 128;
constexpr std::size_t kMaximumFieldCount = 8;
constexpr uint32_t kMinimumRefreshSeconds = 30;
constexpr uint32_t kMaximumRefreshSeconds = 86400;
constexpr uint32_t kMinimumRotationSeconds = 5;
constexpr uint32_t kMaximumRotationSeconds = 300;

using JsonPointer = std::unique_ptr<cJSON, decltype(&cJSON_Delete)>;

template <typename T>
ParseResult<T> failure(const char* code) {
    return {std::nullopt, code};
}

bool isHex(char value) {
    return (value >= '0' && value <= '9') || (value >= 'a' && value <= 'f') ||
           (value >= 'A' && value <= 'F');
}

bool hasEmbeddedNullEscape(const std::string& input) {
    bool inString = false;
    for (std::size_t index = 0; index < input.size(); ++index) {
        const char value = input[index];
        if (!inString) {
            if (value == '"') inString = true;
            continue;
        }
        if (value == '"') {
            inString = false;
            continue;
        }
        if (value != '\\' || index + 1 >= input.size()) continue;
        const char escaped = input[++index];
        if (escaped != 'u') continue;
        if (index + 4 >= input.size()) continue;
        bool allZero = true;
        bool allHex = true;
        for (std::size_t digit = index + 1; digit <= index + 4; ++digit) {
            allHex = allHex && isHex(input[digit]);
            allZero = allZero && input[digit] == '0';
        }
        if (allHex && allZero) return true;
        index += 4;
    }
    return false;
}

bool validateJsonNesting(const std::string& input) {
    std::size_t depth = 0;
    bool inString = false;
    bool escaped = false;
    for (const char value : input) {
        if (inString) {
            if (escaped) {
                escaped = false;
            } else if (value == '\\') {
                escaped = true;
            } else if (value == '"') {
                inString = false;
            }
            continue;
        }
        if (value == '"') {
            inString = true;
        } else if (value == '{' || value == '[') {
            if (++depth > kMaximumJsonDepth) return false;
        } else if (value == '}' || value == ']') {
            if (depth > 0) --depth;
        }
    }
    return true;
}

bool jsonWhitespace(char value) {
    return value == ' ' || value == '\t' || value == '\r' || value == '\n';
}

bool digit(char value) {
    return value >= '0' && value <= '9';
}

bool validateJsonNumberLexemes(const std::string& input) {
    bool inString = false;
    bool escaped = false;
    for (std::size_t index = 0; index < input.size();) {
        const char value = input[index];
        if (inString) {
            if (escaped) escaped = false;
            else if (value == '\\') escaped = true;
            else if (value == '"') inString = false;
            ++index;
            continue;
        }
        if (value == '"') {
            inString = true;
            ++index;
            continue;
        }
        if (value == '+' || (value == '.' && index + 1 < input.size() && digit(input[index + 1]))) return false;
        if (value != '-' && !digit(value)) {
            ++index;
            continue;
        }

        std::size_t end = index;
        if (input[end] == '-') {
            if (++end >= input.size()) return false;
        }
        if (input[end] == '0') {
            ++end;
            if (end < input.size() && digit(input[end])) return false;
        } else if (input[end] >= '1' && input[end] <= '9') {
            while (end < input.size() && digit(input[end])) ++end;
        } else {
            return false;
        }
        if (end < input.size() && input[end] == '.') {
            ++end;
            if (end >= input.size() || !digit(input[end])) return false;
            while (end < input.size() && digit(input[end])) ++end;
        }
        if (end < input.size() && (input[end] == 'e' || input[end] == 'E')) {
            ++end;
            if (end < input.size() && (input[end] == '+' || input[end] == '-')) ++end;
            if (end >= input.size() || !digit(input[end])) return false;
            while (end < input.size() && digit(input[end])) ++end;
        }
        if (end < input.size() && !jsonWhitespace(input[end]) && input[end] != ',' &&
            input[end] != ']' && input[end] != '}') return false;
        index = end;
    }
    return true;
}

bool hasDuplicateKeys(const cJSON* node) {
    if (cJSON_IsObject(node)) {
        std::set<std::string> keys;
        for (const cJSON* child = node->child; child != nullptr; child = child->next) {
            if (child->string == nullptr || !keys.insert(child->string).second) return true;
            if (hasDuplicateKeys(child)) return true;
        }
    } else if (cJSON_IsArray(node)) {
        for (const cJSON* child = node->child; child != nullptr; child = child->next) {
            if (hasDuplicateKeys(child)) return true;
        }
    }
    return false;
}

struct ParsedJson {
    JsonPointer root{nullptr, &cJSON_Delete};
    std::string error;
};

ParsedJson parseJson(const std::string& input, std::size_t maximumBytes) {
    if (input.size() > maximumBytes) return {JsonPointer(nullptr, &cJSON_Delete), "size"};
    if (input.find('\0') != std::string::npos || hasEmbeddedNullEscape(input)) {
        return {JsonPointer(nullptr, &cJSON_Delete), "string"};
    }
    if (!validateJsonNesting(input)) return {JsonPointer(nullptr, &cJSON_Delete), "depth"};
    if (!validateJsonNumberLexemes(input)) return {JsonPointer(nullptr, &cJSON_Delete), "number"};
    const char* end = nullptr;
    JsonPointer root(cJSON_ParseWithOpts(input.c_str(), &end, 0), &cJSON_Delete);
    if (!root) return {JsonPointer(nullptr, &cJSON_Delete), "json"};
    if (end == nullptr) return {JsonPointer(nullptr, &cJSON_Delete), "json"};
    while (*end == ' ' || *end == '\t' || *end == '\r' || *end == '\n') ++end;
    if (end != input.c_str() + input.size()) return {JsonPointer(nullptr, &cJSON_Delete), "trailing"};
    if (hasDuplicateKeys(root.get())) return {JsonPointer(nullptr, &cJSON_Delete), "duplicate"};
    return {std::move(root), {}};
}

const cJSON* field(const cJSON* object, const char* name) {
    return cJSON_GetObjectItemCaseSensitive(object, name);
}

bool isNull(const cJSON* node) {
    return node != nullptr && cJSON_IsNull(node);
}

bool controlsPresent(const std::string& value) {
    for (const unsigned char character : value) {
        if (character < 0x20 || character == 0x7f) return true;
    }
    return false;
}

std::optional<std::size_t> utf16Length(const std::string& value) {
    std::size_t units = 0;
    for (std::size_t index = 0; index < value.size();) {
        const unsigned char first = static_cast<unsigned char>(value[index]);
        if (first <= 0x7f) {
            ++index;
            ++units;
            continue;
        }
        if (first >= 0xc2 && first <= 0xdf) {
            if (index + 1 >= value.size()) return std::nullopt;
            const unsigned char second = static_cast<unsigned char>(value[index + 1]);
            if (second < 0x80 || second > 0xbf) return std::nullopt;
            index += 2;
            ++units;
            continue;
        }
        if (first >= 0xe0 && first <= 0xef) {
            if (index + 2 >= value.size()) return std::nullopt;
            const unsigned char second = static_cast<unsigned char>(value[index + 1]);
            const unsigned char third = static_cast<unsigned char>(value[index + 2]);
            const bool secondValid = first == 0xe0 ? (second >= 0xa0 && second <= 0xbf)
                : first == 0xed ? (second >= 0x80 && second <= 0x9f)
                : (second >= 0x80 && second <= 0xbf);
            if (!secondValid || third < 0x80 || third > 0xbf) return std::nullopt;
            index += 3;
            ++units;
            continue;
        }
        if (first >= 0xf0 && first <= 0xf4) {
            if (index + 3 >= value.size()) return std::nullopt;
            const unsigned char second = static_cast<unsigned char>(value[index + 1]);
            const unsigned char third = static_cast<unsigned char>(value[index + 2]);
            const unsigned char fourth = static_cast<unsigned char>(value[index + 3]);
            const bool secondValid = first == 0xf0 ? (second >= 0x90 && second <= 0xbf)
                : first == 0xf4 ? (second >= 0x80 && second <= 0x8f)
                : (second >= 0x80 && second <= 0xbf);
            if (!secondValid || third < 0x80 || third > 0xbf || fourth < 0x80 || fourth > 0xbf) return std::nullopt;
            index += 4;
            units += 2;
            continue;
        }
        return std::nullopt;
    }
    return units;
}

bool printableAscii(const std::string& value, bool allowSpace) {
    for (const unsigned char character : value) {
        if (character < (allowSpace ? 0x20 : 0x21) || character > 0x7e) return false;
    }
    return true;
}

bool readString(const cJSON* node, std::string& output, bool allowEmpty = true) {
    if (!cJSON_IsString(node) || node->valuestring == nullptr) return false;
    output.assign(node->valuestring);
    return (allowEmpty || !output.empty()) && !controlsPresent(output) && utf16Length(output).has_value();
}

bool readNumber(const cJSON* node, double& output) {
    if (!cJSON_IsNumber(node)) return false;
    output = node->valuedouble;
    return std::isfinite(output);
}

bool readInteger(const cJSON* node, int64_t minimum, int64_t maximum, int64_t& output) {
    double number = 0;
    if (!readNumber(node, number) || std::floor(number) != number ||
        number < static_cast<double>(minimum) || number > static_cast<double>(maximum)) {
        return false;
    }
    output = static_cast<int64_t>(number);
    return true;
}

bool leapYear(int year) {
    return year % 4 == 0 && (year % 100 != 0 || year % 400 == 0);
}

int daysInMonth(int year, int month) {
    static constexpr int days[] = {0, 31, 28, 31, 30, 31, 30, 31, 31, 30, 31, 30, 31};
    if (month == 2) return days[month] + (leapYear(year) ? 1 : 0);
    if (month < 1 || month > 12) return 0;
    return days[month];
}

int64_t daysFromCivil(int year, unsigned month, unsigned day) {
    year -= month <= 2;
    const int era = (year >= 0 ? year : year - 399) / 400;
    const unsigned yearOfEra = static_cast<unsigned>(year - era * 400);
    const unsigned adjustedMonth = month > 2 ? month - 3 : month + 9;
    const unsigned dayOfYear = (153 * adjustedMonth + 2) / 5 + day - 1;
    const unsigned dayOfEra = yearOfEra * 365 + yearOfEra / 4 - yearOfEra / 100 + dayOfYear;
    return static_cast<int64_t>(era) * 146097 + static_cast<int64_t>(dayOfEra) - 719468;
}

bool readDigits(const std::string& value, std::size_t& index, std::size_t count, int& output) {
    if (index + count > value.size()) return false;
    int parsed = 0;
    for (std::size_t offset = 0; offset < count; ++offset) {
        const char digit = value[index + offset];
        if (digit < '0' || digit > '9') return false;
        parsed = parsed * 10 + digit - '0';
    }
    index += count;
    output = parsed;
    return true;
}

struct CalendarTimestamp {
    int64_t localEpoch = 0;
    std::optional<int32_t> explicitOffset;
    bool valid = false;
};

CalendarTimestamp parseTimestampParts(const std::string& value, bool allowNaive) {
    CalendarTimestamp parsed;
    std::size_t index = 0;
    int year = 0;
    int month = 0;
    int day = 0;
    int hour = 0;
    int minute = 0;
    int second = 0;
    bool hasSeconds = false;
    if (!readDigits(value, index, 4, year) || year == 0 || index >= value.size() || value[index++] != '-' ||
        !readDigits(value, index, 2, month) || index >= value.size() || value[index++] != '-' ||
        !readDigits(value, index, 2, day) || index >= value.size() ||
        (value[index++] != 'T' && value[index - 1] != 't') || !readDigits(value, index, 2, hour) ||
        index >= value.size() || value[index++] != ':' || !readDigits(value, index, 2, minute)) {
        return parsed;
    }
    if (index < value.size() && value[index] == ':') {
        ++index;
        if (!readDigits(value, index, 2, second)) return parsed;
        hasSeconds = true;
    } else if (!allowNaive) {
        return parsed;
    }
    if (year < 1 || month < 1 || month > 12 || day < 1 || day > daysInMonth(year, month) ||
        hour > 23 || minute > 59 || second > 59) {
        return parsed;
    }
    if (index < value.size() && value[index] == '.') {
        if (!hasSeconds) return parsed;
        ++index;
        const std::size_t start = index;
        while (index < value.size() && value[index] >= '0' && value[index] <= '9') ++index;
        const std::size_t digits = index - start;
        if (digits == 0 || digits > 9) return parsed;
    }
    const int64_t days = daysFromCivil(year, static_cast<unsigned>(month), static_cast<unsigned>(day));
    parsed.localEpoch = days * 86400 + hour * 3600 + minute * 60 + second;
    if (index == value.size()) {
        parsed.valid = allowNaive;
        return parsed;
    }
    if (value[index] == 'Z' || value[index] == 'z') {
        if (++index != value.size()) return parsed;
        parsed.explicitOffset = 0;
        parsed.valid = true;
        return parsed;
    }
    if (value[index] != '+' && value[index] != '-') return parsed;
    const int sign = value[index++] == '+' ? 1 : -1;
    int offsetHour = 0;
    int offsetMinute = 0;
    if (!readDigits(value, index, 2, offsetHour) || index >= value.size() || value[index++] != ':' ||
        !readDigits(value, index, 2, offsetMinute) || index != value.size() || offsetHour > 14 ||
        offsetMinute > 59 || (offsetHour == 14 && offsetMinute != 0)) {
        return parsed;
    }
    parsed.explicitOffset = static_cast<int32_t>(sign * (offsetHour * 3600 + offsetMinute * 60));
    parsed.valid = true;
    return parsed;
}

bool epochFromTimestamp(const std::string& value, bool allowNaive,
                        const std::optional<int32_t>& fallbackOffset, int64_t& epoch,
                        bool& known) {
    const CalendarTimestamp parts = parseTimestampParts(value, allowNaive);
    if (!parts.valid) return false;
    const auto offset = parts.explicitOffset ? parts.explicitOffset : fallbackOffset;
    if (!offset) {
        known = false;
        return true;
    }
    epoch = parts.localEpoch - *offset;
    known = true;
    return true;
}

bool parseRequiredTimestamp(const cJSON* node, int64_t& epoch) {
    std::string value;
    bool known = false;
    return readString(node, value, false) && epochFromTimestamp(value, false, std::nullopt, epoch, known) && known;
}

bool parseOptionalTimestamp(const cJSON* node, std::optional<int64_t>& epoch) {
    if (node == nullptr || isNull(node)) {
        epoch.reset();
        return true;
    }
    int64_t parsed = 0;
    if (!parseRequiredTimestamp(node, parsed)) return false;
    epoch = parsed;
    return true;
}

bool validateUsageWindow(const cJSON* node, std::optional<UsageWindow>& output) {
    if (node == nullptr || isNull(node)) {
        output.reset();
        return true;
    }
    if (!cJSON_IsObject(node)) return false;
    const cJSON* percentNode = field(node, "usedPercent");
    double percent = 0;
    if (!readNumber(percentNode, percent) || percent < 0.0 || percent > 100.0) return false;
    UsageWindow window;
    window.usedPercent = percent;
    const cJSON* minutesNode = field(node, "windowMinutes");
    if (minutesNode != nullptr && !isNull(minutesNode)) {
        double minutes = 0;
        if (!readNumber(minutesNode, minutes) || minutes <= 0.0) return false;
        window.windowMinutes = minutes;
    }
    if (!parseOptionalTimestamp(field(node, "resetsAt"), window.resetsAt)) return false;
    output = window;
    return true;
}

bool validStockString(const cJSON* node, std::string& output, std::size_t maximum, bool required) {
    if (node == nullptr) return !required;
    if (!readString(node, output, !required)) return false;
    const auto length = utf16Length(output);
    if (!length || *length > maximum) return false;
    return !required || !output.empty();
}

bool isWeatherCode(int value) {
    static constexpr int validCodes[] = {0, 1, 2, 3, 45, 48, 51, 53, 55, 56, 57,
                                         61, 63, 65, 66, 67, 71, 73, 75, 77,
                                         80, 81, 82, 85, 86, 95, 96, 99};
    return std::find(std::begin(validCodes), std::end(validCodes), value) != std::end(validCodes);
}

struct UrlParts {
    std::string scheme;
    std::string host;
    uint16_t port = 0;
    bool hasPort = false;
};

std::string lowerAscii(std::string value) {
    for (char& character : value) {
        if (character >= 'A' && character <= 'Z') character = static_cast<char>(character - 'A' + 'a');
    }
    return value;
}

bool validUrl(const std::string& url, UrlParts* parts) {
    if (url.empty() || url.size() > kMaximumUrlLength || controlsPresent(url)) return false;
    for (const unsigned char character : url) {
        if (character <= 0x20 || character == 0x7f || character == '\\') return false;
    }
    const std::size_t schemeEnd = url.find("://");
    if (schemeEnd == std::string::npos || schemeEnd == 0) return false;
    std::string scheme = lowerAscii(url.substr(0, schemeEnd));
    if (scheme != "http" && scheme != "https") return false;
    const std::size_t authorityStart = schemeEnd + 3;
    const std::size_t authorityEnd = url.find_first_of("/?#", authorityStart);
    const std::string authority = url.substr(authorityStart,
        authorityEnd == std::string::npos ? std::string::npos : authorityEnd - authorityStart);
    if (authority.empty() || authority.find('@') != std::string::npos || url.find('#') != std::string::npos) return false;

    std::string host;
    std::string portText;
    bool hasPort = false;
    if (authority.front() == '[') {
        const std::size_t closing = authority.find(']');
        if (closing == std::string::npos || closing == 1) return false;
        host = lowerAscii(authority.substr(0, closing + 1));
        const std::string address = authority.substr(1, closing - 1);
        if (address.find(':') == std::string::npos) return false;
        for (const unsigned char character : address) {
            if (!((character >= '0' && character <= '9') || (character >= 'a' && character <= 'f') ||
                  (character >= 'A' && character <= 'F') || character == ':' || character == '.')) return false;
        }
        if (closing + 1 < authority.size()) {
            if (authority[closing + 1] != ':') return false;
            portText = authority.substr(closing + 2);
            hasPort = true;
        }
    } else {
        const std::size_t colon = authority.rfind(':');
        if (colon != std::string::npos) {
            if (authority.find(':') != colon) return false;
            host = lowerAscii(authority.substr(0, colon));
            portText = authority.substr(colon + 1);
            hasPort = true;
        } else {
            host = lowerAscii(authority);
        }
        if (host.empty()) return false;
        for (const unsigned char character : host) {
            if (!((character >= 'a' && character <= 'z') || (character >= '0' && character <= '9') ||
                  character == '.' || character == '-' || character == '_')) return false;
        }
    }
    uint16_t port = 0;
    if (hasPort) {
        if (portText.empty()) return false;
        unsigned parsed = 0;
        for (const char digit : portText) {
            if (digit < '0' || digit > '9') return false;
            parsed = parsed * 10U + static_cast<unsigned>(digit - '0');
            if (parsed > 65535U) return false;
        }
        if (parsed == 0) return false;
        port = static_cast<uint16_t>(parsed);
    }
    if (parts != nullptr) *parts = {scheme, host, port, hasPort};
    return true;
}

uint16_t normalizedPort(const UrlParts& url) {
    if (url.hasPort) return url.port;
    return url.scheme == "https" ? 443 : 80;
}

std::optional<std::vector<std::pair<std::string, std::size_t>>> parsePath(const std::string& path) {
    if (path.empty() || path.size() > kMaximumMappingLength) return std::nullopt;
    std::vector<std::pair<std::string, std::size_t>> steps;
    std::size_t index = 0;
    while (index < path.size()) {
        const std::size_t start = index;
        const unsigned char first = static_cast<unsigned char>(path[index]);
        if (!((first >= 'A' && first <= 'Z') || (first >= 'a' && first <= 'z') || first == '_')) return std::nullopt;
        ++index;
        while (index < path.size()) {
            const unsigned char character = static_cast<unsigned char>(path[index]);
            if (!((character >= 'A' && character <= 'Z') || (character >= 'a' && character <= 'z') ||
                  (character >= '0' && character <= '9') || character == '_')) break;
            ++index;
        }
        steps.emplace_back(path.substr(start, index - start), std::numeric_limits<std::size_t>::max());
        while (index < path.size() && path[index] == '[') {
            ++index;
            if (index >= path.size() || path[index] < '0' || path[index] > '9') return std::nullopt;
            std::size_t subscript = 0;
            while (index < path.size() && path[index] >= '0' && path[index] <= '9') {
                const unsigned digit = static_cast<unsigned>(path[index++] - '0');
                if (subscript > 100000U) return std::nullopt;
                subscript = subscript * 10U + digit;
            }
            if (index >= path.size() || path[index++] != ']' || subscript > 100000U) return std::nullopt;
            steps.emplace_back(std::string(), subscript);
        }
        if (index == path.size()) break;
        if (path[index++] != '.' || index == path.size()) return std::nullopt;
    }
    return steps;
}

const cJSON* lookupPath(const cJSON* root, const std::vector<std::pair<std::string, std::size_t>>& steps) {
    const cJSON* current = root;
    for (const auto& step : steps) {
        if (step.second == std::numeric_limits<std::size_t>::max()) {
            current = field(current, step.first.c_str());
        } else if (cJSON_IsArray(current)) {
            current = cJSON_GetArrayItem(current, static_cast<int>(step.second));
        } else {
            return nullptr;
        }
        if (current == nullptr) return nullptr;
    }
    return current;
}

const std::map<std::string, std::string>& defaultWeatherFields() {
    static const std::map<std::string, std::string> fields = {
        {"temperature", "current.temperature_2m"},
        {"humidity", "current.relative_humidity_2m"},
        {"weatherCode", "current.weather_code"},
        {"updatedAt", "current.time"},
        {"utcOffsetSeconds", "utc_offset_seconds"},
        {"hourlyTimes", "hourly.time"},
        {"hourlyTemperatures", "hourly.temperature_2m"},
        {"hourlyRainProbability", "hourly.precipitation_probability"},
    };
    return fields;
}

bool resolveMappings(const SourceConfig& source, std::map<std::string, std::string>& mappings) {
    mappings = defaultWeatherFields();
    for (const auto& item : source.fields) {
        if (defaultWeatherFields().find(item.first) == defaultWeatherFields().end() ||
            !parsePath(item.second)) return false;
        mappings[item.first] = item.second;
    }
    return mappings.size() == defaultWeatherFields().size();
}

const cJSON* mapped(const cJSON* root, const std::map<std::string, std::string>& mappings,
                    const char* name) {
    const auto iterator = mappings.find(name);
    if (iterator == mappings.end()) return nullptr;
    const auto path = parsePath(iterator->second);
    return path ? lookupPath(root, *path) : nullptr;
}

bool parseMappedOptionalNumber(const cJSON* root, const std::map<std::string, std::string>& mappings,
                               const char* name, std::optional<double>& output) {
    const cJSON* node = mapped(root, mappings, name);
    if (node == nullptr || isNull(node)) {
        output.reset();
        return true;
    }
    double number = 0;
    if (!readNumber(node, number)) return false;
    output = number;
    return true;
}

bool parseMappedArray(const cJSON* root, const std::map<std::string, std::string>& mappings,
                      const char* name, const cJSON*& array, bool& scalar) {
    array = mapped(root, mappings, name);
    scalar = false;
    if (array == nullptr || isNull(array)) return true;
    if (cJSON_IsArray(array)) return true;
    scalar = true;
    return true;
}

std::size_t arrayLength(const cJSON* array, bool scalar) {
    if (array == nullptr || isNull(array)) return 0;
    return scalar ? 1 : static_cast<std::size_t>(cJSON_GetArraySize(array));
}

const cJSON* arrayValue(const cJSON* array, bool scalar, std::size_t index) {
    if (array == nullptr || isNull(array)) return nullptr;
    if (scalar) return index == 0 ? array : nullptr;
    return cJSON_GetArrayItem(array, static_cast<int>(index));
}

bool parseWeatherTime(const cJSON* node, const std::optional<int32_t>& offset,
                      std::optional<int64_t>& output) {
    if (node == nullptr || isNull(node)) {
        output.reset();
        return true;
    }
    std::string text;
    if (!readString(node, text, false)) return false;
    int64_t epoch = 0;
    bool known = false;
    if (!epochFromTimestamp(text, true, offset, epoch, known)) return false;
    if (known) output = epoch;
    else output.reset();
    return true;
}

bool isHeaderNameCharacter(unsigned char character) {
    return (character >= 'a' && character <= 'z') || (character >= 'A' && character <= 'Z') ||
           (character >= '0' && character <= '9') || std::string_view("!#$%&'*+-.^_`|~").find(static_cast<char>(character)) != std::string_view::npos;
}

bool validHeader(const std::string& name, const std::string& value) {
    if (name.empty() || name.size() > kMaximumHeaderNameLength || value.size() > kMaximumHeaderValueLength ||
        controlsPresent(value)) return false;
    return std::all_of(name.begin(), name.end(), [](unsigned char character) {
        return isHeaderNameCharacter(character);
    });
}

bool validMappingName(const std::string& name) {
    return defaultWeatherFields().find(name) != defaultWeatherFields().end();
}

bool validateSource(const SourceConfig& source, bool weather) {
    if (!source.url.empty() && !validUrl(source.url, nullptr)) return false;
    if (source.url.size() > kMaximumUrlLength || source.headers.size() > kMaximumHeaderCount ||
        source.refreshSeconds < kMinimumRefreshSeconds || source.refreshSeconds > kMaximumRefreshSeconds) return false;
    for (const auto& header : source.headers) {
        if (!validHeader(header.first, header.second)) return false;
    }
    if (weather) {
        if (source.label.size() > 48 || controlsPresent(source.label) || source.fields.size() > kMaximumFieldCount) return false;
        for (const auto& mapping : source.fields) {
            if (!validMappingName(mapping.first) || !parsePath(mapping.second)) return false;
        }
    } else if (!source.label.empty() || !source.fields.empty()) {
        return false;
    }
    return true;
}

bool validateConfig(const Config& config) {
    if (config.ssid.size() > 32 || controlsPresent(config.ssid) || config.wifiPassword.size() > 64 ||
        controlsPresent(config.wifiPassword)) return false;
    if (!config.usageToken.empty() &&
        (config.usageToken.size() < 16 || config.usageToken.size() > 128 || !printableAscii(config.usageToken, false))) return false;
    if (!config.setupPassword.empty() &&
        (config.setupPassword.size() < 8 || config.setupPassword.size() > 64 || !printableAscii(config.setupPassword, true))) return false;
    if (config.autoRotateSeconds != 0 &&
        (config.autoRotateSeconds < kMinimumRotationSeconds || config.autoRotateSeconds > kMaximumRotationSeconds)) return false;
    if (config.touch.minX < 0 || config.touch.minX > 4095 || config.touch.maxX < 0 || config.touch.maxX > 4095 ||
        config.touch.minY < 0 || config.touch.minY > 4095 || config.touch.maxY < 0 || config.touch.maxY > 4095 ||
        config.touch.minX >= config.touch.maxX || config.touch.minY >= config.touch.maxY) return false;
    return validateSource(config.weather, true) && validateSource(config.stocks, false);
}

bool parseHeaderPatch(const cJSON* node, SourceConfig& source) {
    if (!cJSON_IsObject(node) || cJSON_GetArraySize(node) > static_cast<int>(kMaximumHeaderCount)) return false;
    std::map<std::string, std::string> headers;
    for (const cJSON* item = node->child; item != nullptr; item = item->next) {
        std::string value;
        if (item->string == nullptr || !readString(item, value) || !validHeader(item->string, value)) return false;
        headers.emplace(item->string, std::move(value));
    }
    source.headers = std::move(headers);
    return true;
}

bool parseFieldsPatch(const cJSON* node, SourceConfig& source) {
    if (!cJSON_IsObject(node) || cJSON_GetArraySize(node) > static_cast<int>(kMaximumFieldCount)) return false;
    auto fields = source.fields;
    for (const cJSON* item = node->child; item != nullptr; item = item->next) {
        std::string path;
        if (item->string == nullptr || !validMappingName(item->string) || !readString(item, path, false) ||
            !parsePath(path)) return false;
        fields[item->string] = std::move(path);
    }
    source.fields = std::move(fields);
    return source.fields.size() <= kMaximumFieldCount;
}

bool parseSourcePatch(const cJSON* node, SourceConfig& source, bool weather) {
    if (!cJSON_IsObject(node)) return false;
    for (const cJSON* item = node->child; item != nullptr; item = item->next) {
        if (item->string == nullptr) return false;
        const std::string key(item->string);
        if (key == "url") {
            if (!readString(item, source.url) || source.url.size() > kMaximumUrlLength ||
                (!source.url.empty() && !validUrl(source.url, nullptr))) return false;
        } else if (key == "headers") {
            if (!parseHeaderPatch(item, source)) return false;
        } else if (key == "refreshSeconds") {
            int64_t seconds = 0;
            if (!readInteger(item, kMinimumRefreshSeconds, kMaximumRefreshSeconds, seconds)) return false;
            source.refreshSeconds = static_cast<uint32_t>(seconds);
        } else if (weather && key == "label") {
            if (!readString(item, source.label) || source.label.size() > 48) return false;
        } else if (weather && key == "fields") {
            if (!parseFieldsPatch(item, source)) return false;
        } else {
            return false;
        }
    }
    return true;
}

bool parseTouchPatch(const cJSON* node, TouchCalibration& touch) {
    if (!cJSON_IsObject(node)) return false;
    for (const cJSON* item = node->child; item != nullptr; item = item->next) {
        if (item->string == nullptr) return false;
        const std::string key(item->string);
        if (key == "swapAxes" || key == "invertX" || key == "invertY") {
            if (!cJSON_IsBool(item)) return false;
            const bool value = cJSON_IsTrue(item);
            if (key == "swapAxes") touch.swapAxes = value;
            else if (key == "invertX") touch.invertX = value;
            else touch.invertY = value;
        } else {
            int64_t value = 0;
            if (!readInteger(item, 0, 4095, value)) return false;
            if (key == "minX") touch.minX = static_cast<int>(value);
            else if (key == "maxX") touch.maxX = static_cast<int>(value);
            else if (key == "minY") touch.minY = static_cast<int>(value);
            else if (key == "maxY") touch.maxY = static_cast<int>(value);
            else return false;
        }
    }
    return true;
}

bool parseConfigObject(const cJSON* root, Config& config, bool allowSetupPassword) {
    if (!cJSON_IsObject(root)) return false;
    for (const cJSON* item = root->child; item != nullptr; item = item->next) {
        if (item->string == nullptr) return false;
        const std::string key(item->string);
        if (key == "ssid") {
            if (!readString(item, config.ssid) || config.ssid.size() > 32) return false;
        } else if (key == "wifiPassword") {
            if (!readString(item, config.wifiPassword) || config.wifiPassword.size() > 64) return false;
        } else if (key == "usageToken") {
            if (!readString(item, config.usageToken)) return false;
        } else if (key == "setupPassword" && allowSetupPassword) {
            if (!readString(item, config.setupPassword)) return false;
        } else if (key == "weather") {
            if (!parseSourcePatch(item, config.weather, true)) return false;
        } else if (key == "stocks") {
            if (!parseSourcePatch(item, config.stocks, false)) return false;
        } else if (key == "autoRotateSeconds") {
            int64_t seconds = 0;
            if (!readInteger(item, 0, kMaximumRotationSeconds, seconds)) return false;
            config.autoRotateSeconds = static_cast<uint32_t>(seconds);
        } else if (key == "touch") {
            if (!parseTouchPatch(item, config.touch)) return false;
        } else {
            return false;
        }
    }
    return validateConfig(config);
}

void addString(cJSON* object, const char* name, const std::string& value) {
    cJSON_AddStringToObject(object, name, value.c_str());
}

void addNumber(cJSON* object, const char* name, double value) {
    cJSON_AddNumberToObject(object, name, value);
}

cJSON* configJson(const Config& config, bool includeSecrets, bool publicUrls) {
    cJSON* root = cJSON_CreateObject();
    if (root == nullptr) return nullptr;
    if (includeSecrets) {
        addString(root, "ssid", config.ssid);
        addString(root, "wifiPassword", config.wifiPassword);
        addString(root, "usageToken", config.usageToken);
        addString(root, "setupPassword", config.setupPassword);
    }
    cJSON* weather = cJSON_AddObjectToObject(root, "weather");
    cJSON* stocks = cJSON_AddObjectToObject(root, "stocks");
    cJSON* touch = cJSON_AddObjectToObject(root, "touch");
    if (weather == nullptr || stocks == nullptr || touch == nullptr) {
        cJSON_Delete(root);
        return nullptr;
    }
    auto sourceToJson = [includeSecrets, publicUrls](cJSON* target, const SourceConfig& source, bool isWeather) {
        std::string url = source.url;
        if (publicUrls) {
            const std::size_t query = url.find_first_of("?#");
            if (query != std::string::npos) url.resize(query);
        }
        addString(target, "url", url);
        addNumber(target, "refreshSeconds", source.refreshSeconds);
        if (isWeather) {
            addString(target, "label", source.label);
            cJSON* fields = cJSON_AddObjectToObject(target, "fields");
            if (fields != nullptr) {
                for (const auto& mapping : source.fields) addString(fields, mapping.first.c_str(), mapping.second);
            }
        }
        if (includeSecrets) {
            cJSON* headers = cJSON_AddObjectToObject(target, "headers");
            if (headers != nullptr) {
                for (const auto& header : source.headers) addString(headers, header.first.c_str(), header.second);
            }
        }
    };
    sourceToJson(weather, config.weather, true);
    sourceToJson(stocks, config.stocks, false);
    addNumber(root, "autoRotateSeconds", config.autoRotateSeconds);
    cJSON_AddBoolToObject(touch, "swapAxes", config.touch.swapAxes);
    cJSON_AddBoolToObject(touch, "invertX", config.touch.invertX);
    cJSON_AddBoolToObject(touch, "invertY", config.touch.invertY);
    addNumber(touch, "minX", config.touch.minX);
    addNumber(touch, "maxX", config.touch.maxX);
    addNumber(touch, "minY", config.touch.minY);
    addNumber(touch, "maxY", config.touch.maxY);
    return root;
}

std::string printJson(cJSON* root) {
    if (root == nullptr) return {};
    char* output = cJSON_PrintUnformatted(root);
    cJSON_Delete(root);
    if (output == nullptr) return {};
    std::string result(output);
    cJSON_free(output);
    return result;
}

bool sameSource(const SourceConfig& left, const SourceConfig& right) {
    return left.url == right.url && left.headers == right.headers &&
           left.refreshSeconds == right.refreshSeconds && left.fields == right.fields;
}

template <typename T>
SourceSnapshot<T> statusAt(const SourceSnapshot<T>& current, uint64_t nowMs, uint32_t refreshSeconds) {
    SourceSnapshot<T> result = current;
    result.unknown = !result.data.has_value();
    if (!result.data) {
        result.stale = false;
        result.receivedAtMs = 0;
        return result;
    }
    const uint64_t ageMs = nowMs >= result.receivedAtMs ? nowMs - result.receivedAtMs : 0;
    const uint64_t staleAfter = static_cast<uint64_t>(refreshSeconds) * 2000ULL;
    result.stale = result.stale || ageMs >= staleAfter;
    return result;
}

template <typename T>
void accept(SourceSnapshot<T>& target, const T& data, uint64_t nowMs) {
    target.data = data;
    target.receivedAtMs = nowMs;
    target.unknown = false;
    target.stale = false;
}

}  // namespace

ParseResult<Usage> parseUsage(const std::string& json) {
    ParsedJson parsed = parseJson(json, kUsageBodyLimit);
    if (!parsed.root) return failure<Usage>(parsed.error.c_str());
    const cJSON* root = parsed.root.get();
    if (!cJSON_IsObject(root)) return failure<Usage>("shape");
    int64_t schema = 0;
    if (!readInteger(field(root, "schemaVersion"), 1, 1, schema)) return failure<Usage>("schema");
    std::string type;
    if (!readString(field(root, "type"), type, false) || type != "usage") return failure<Usage>("type");
    Usage result;
    if (!readString(field(root, "provider"), result.provider, false) ||
        (result.provider != "claude" && result.provider != "codex")) return failure<Usage>("provider");
    if (!parseRequiredTimestamp(field(root, "updatedAt"), result.updatedAt)) return failure<Usage>("timestamp");
    if (!validateUsageWindow(field(root, "primary"), result.primary) ||
        !validateUsageWindow(field(root, "secondary"), result.secondary)) return failure<Usage>("window");
    return {std::move(result), {}};
}

ParseResult<Stocks> parseStocks(const std::string& json) {
    ParsedJson parsed = parseJson(json, kSourceBodyLimit);
    if (!parsed.root) return failure<Stocks>(parsed.error.c_str());
    const cJSON* root = parsed.root.get();
    if (!cJSON_IsObject(root) || field(root, "error") != nullptr) return failure<Stocks>("shape");
    int64_t schema = 0;
    if (!readInteger(field(root, "schemaVersion"), 1, 1, schema)) return failure<Stocks>("schema");
    Stocks result;
    if (!parseRequiredTimestamp(field(root, "generatedAt"), result.generatedAt)) return failure<Stocks>("timestamp");
    const cJSON* items = field(root, "items");
    if (!cJSON_IsArray(items) || cJSON_GetArraySize(items) > static_cast<int>(result.items.size())) return failure<Stocks>("items");
    const int itemCount = cJSON_GetArraySize(items);
    for (int index = 0; index < itemCount; ++index) {
        const cJSON* node = cJSON_GetArrayItem(items, index);
        if (!cJSON_IsObject(node)) return failure<Stocks>("item");
        Stock item;
        if (!validStockString(field(node, "symbol"), item.symbol, 32, true)) return failure<Stocks>("symbol");
        if (!validStockString(field(node, "label"), item.label, 64, false)) return failure<Stocks>("label");
        if (!validStockString(field(node, "currency"), item.currency, 8, false)) return failure<Stocks>("currency");
        const cJSON* priceNode = field(node, "price");
        if (priceNode != nullptr && !isNull(priceNode)) {
            double price = 0;
            if (!readNumber(priceNode, price) || price < 0.0) return failure<Stocks>("price");
            item.price = price;
        }
        const cJSON* changeNode = field(node, "changePercent");
        if (changeNode != nullptr && !isNull(changeNode)) {
            double change = 0;
            if (!readNumber(changeNode, change)) return failure<Stocks>("change");
            item.changePercent = change;
        }
        if (!parseOptionalTimestamp(field(node, "quotedAt"), item.quotedAt)) return failure<Stocks>("quoted_at");
        result.items[static_cast<std::size_t>(index)] = std::move(item);
    }
    result.count = static_cast<std::size_t>(itemCount);
    return {std::move(result), {}};
}

ParseResult<Weather> parseWeather(const std::string& json, const SourceConfig& source) {
    ParsedJson parsed = parseJson(json, kSourceBodyLimit);
    if (!parsed.root) return failure<Weather>(parsed.error.c_str());
    const cJSON* root = parsed.root.get();
    if (!cJSON_IsObject(root)) return failure<Weather>("shape");
    const cJSON* errorNode = field(root, "error");
    if (errorNode != nullptr && !cJSON_IsFalse(errorNode)) return failure<Weather>("upstream_error");
    std::map<std::string, std::string> mappings;
    if (!resolveMappings(source, mappings)) return failure<Weather>("mapping");
    Weather result;
    if (!parseMappedOptionalNumber(root, mappings, "temperature", result.temperature) ||
        (result.temperature && (*result.temperature < -100.0 || *result.temperature > 80.0))) return failure<Weather>("temperature");
    if (!parseMappedOptionalNumber(root, mappings, "humidity", result.humidity) ||
        (result.humidity && (*result.humidity < 0.0 || *result.humidity > 100.0))) return failure<Weather>("humidity");
    const cJSON* codeNode = mapped(root, mappings, "weatherCode");
    if (codeNode != nullptr && !isNull(codeNode)) {
        int64_t code = 0;
        if (!readInteger(codeNode, 0, 99, code) || !isWeatherCode(static_cast<int>(code))) return failure<Weather>("weather_code");
        result.weatherCode = static_cast<int>(code);
    }
    const cJSON* offsetNode = mapped(root, mappings, "utcOffsetSeconds");
    if (offsetNode != nullptr && !isNull(offsetNode)) {
        int64_t offset = 0;
        if (!readInteger(offsetNode, -50400, 50400, offset)) return failure<Weather>("utc_offset");
        result.utcOffsetSeconds = static_cast<int32_t>(offset);
    }
    const cJSON* updatedNode = mapped(root, mappings, "updatedAt");
    if (!parseWeatherTime(updatedNode, result.utcOffsetSeconds, result.updatedAt)) return failure<Weather>("updated_at");

    const cJSON* timeArray = nullptr;
    const cJSON* temperatureArray = nullptr;
    const cJSON* rainArray = nullptr;
    bool timeScalar = false;
    bool temperatureScalar = false;
    bool rainScalar = false;
    if (!parseMappedArray(root, mappings, "hourlyTimes", timeArray, timeScalar) ||
        !parseMappedArray(root, mappings, "hourlyTemperatures", temperatureArray, temperatureScalar) ||
        !parseMappedArray(root, mappings, "hourlyRainProbability", rainArray, rainScalar)) return failure<Weather>("hourly");
    const auto validArray = [](const cJSON* array, bool scalar) {
        return array == nullptr || isNull(array) || scalar || cJSON_IsArray(array);
    };
    if (!validArray(timeArray, timeScalar) || !validArray(temperatureArray, temperatureScalar) ||
        !validArray(rainArray, rainScalar)) return failure<Weather>("hourly");
    const std::size_t count = std::min<std::size_t>(4, std::max({arrayLength(timeArray, timeScalar),
        arrayLength(temperatureArray, temperatureScalar), arrayLength(rainArray, rainScalar)}));
    for (std::size_t index = 0; index < count; ++index) {
        if (!parseWeatherTime(arrayValue(timeArray, timeScalar, index), result.utcOffsetSeconds,
                              result.hourlyTimes[index])) return failure<Weather>("hourly_time");
        const cJSON* tempNode = arrayValue(temperatureArray, temperatureScalar, index);
        if (tempNode != nullptr && !isNull(tempNode)) {
            double temperature = 0;
            if (!readNumber(tempNode, temperature) || temperature < -100.0 || temperature > 80.0) return failure<Weather>("hourly_temperature");
            result.hourlyTemperatures[index] = temperature;
        }
        const cJSON* rainNode = arrayValue(rainArray, rainScalar, index);
        if (rainNode != nullptr && !isNull(rainNode)) {
            double probability = 0;
            if (!readNumber(rainNode, probability) || probability < 0.0 || probability > 100.0) return failure<Weather>("rain_probability");
            result.hourlyRainProbability[index] = probability;
        }
    }
    result.hourlyCount = count;
    bool hasMeasurement = result.temperature.has_value() || result.humidity.has_value() || result.weatherCode.has_value();
    for (std::size_t index = 0; index < result.hourlyCount; ++index) {
        hasMeasurement = hasMeasurement || result.hourlyTemperatures[index].has_value() ||
                         result.hourlyRainProbability[index].has_value();
    }
    if (!hasMeasurement) return failure<Weather>("no_readings");
    return {std::move(result), {}};
}

Config defaultConfig() {
    Config config;
    config.weather.refreshSeconds = 900;
    config.weather.label = "Weather";
    config.weather.fields = defaultWeatherFields();
    config.stocks.refreshSeconds = 300;
    config.autoRotateSeconds = 20;
    config.touch = TouchCalibration{};
    return config;
}

ParseResult<Config> applyConfigPatch(const Config& current, const std::string& json) {
    ParsedJson parsed = parseJson(json, kConfigBodyLimit);
    if (!parsed.root) return failure<Config>(parsed.error.c_str());
    if (field(parsed.root.get(), "setupPassword") != nullptr) return failure<Config>("setup_password");
    Config config = current;
    if (!parseConfigObject(parsed.root.get(), config, false)) return failure<Config>("config");
    return {std::move(config), {}};
}

ParseResult<Config> parseStoredConfig(const std::string& json) {
    ParsedJson parsed = parseJson(json, kStoredConfigLimit);
    if (!parsed.root) return failure<Config>(parsed.error.c_str());
    Config config = defaultConfig();
    if (!parseConfigObject(parsed.root.get(), config, true)) return failure<Config>("stored_config");
    return {std::move(config), {}};
}

std::string serializeConfig(const Config& config) {
    if (!validateConfig(config)) return {};
    std::string serialized = printJson(configJson(config, true, false));
    if (serialized.size() > kStoredConfigLimit) return {};
    return serialized;
}

std::string redactedConfig(const Config& config) {
    return printJson(configJson(config, false, true));
}

void Model::applyConfig(const Config& config) {
    const std::lock_guard<std::mutex> lock(mutex_);
    const bool weatherChanged = !sameSource(config_.weather, config.weather);
    const bool stocksChanged = !sameSource(config_.stocks, config.stocks);
    if (weatherChanged || stocksChanged) {
        ++generation_;
        if (weatherChanged) weather_ = SourceSnapshot<Weather>{};
        if (stocksChanged) stocks_ = SourceSnapshot<Stocks>{};
    }
    config_ = config;
}

void Model::acceptUsage(const Usage& usage, uint64_t nowMs) {
    const std::lock_guard<std::mutex> lock(mutex_);
    if (usage.provider == "claude") {
        if (claude_.data && claude_.data->updatedAt > usage.updatedAt) return;
        accept(claude_, usage, nowMs);
    } else if (usage.provider == "codex") {
        if (codex_.data && codex_.data->updatedAt > usage.updatedAt) return;
        accept(codex_, usage, nowMs);
    }
}

void Model::acceptStocks(const Stocks& stocks, uint64_t nowMs, uint64_t generation) {
    const std::lock_guard<std::mutex> lock(mutex_);
    if (generation != generation_) return;
    accept(stocks_, stocks, nowMs);
}

void Model::acceptWeather(const Weather& weather, uint64_t nowMs, uint64_t generation) {
    const std::lock_guard<std::mutex> lock(mutex_);
    if (generation != generation_) return;
    accept(weather_, weather, nowMs);
}

void Model::markFailed(const std::string& source, uint64_t generation) {
    const std::lock_guard<std::mutex> lock(mutex_);
    if (generation != generation_) return;
    if (source == "stocks") {
        if (stocks_.data) stocks_.stale = true;
    } else if (source == "weather" && weather_.data) {
        weather_.stale = true;
    }
}

Snapshot Model::snapshot(uint64_t nowMs) const {
    const std::lock_guard<std::mutex> lock(mutex_);
    Snapshot result;
    result.claude = claude_;
    result.codex = codex_;
    result.claude.unknown = !result.claude.data.has_value();
    result.codex.unknown = !result.codex.data.has_value();
    result.stocks = statusAt(stocks_, nowMs, config_.stocks.refreshSeconds);
    result.weather = statusAt(weather_, nowMs, config_.weather.refreshSeconds);
    result.weatherLabel = config_.weather.label;
    result.autoRotateSeconds = config_.autoRotateSeconds;
    return result;
}

uint64_t Model::generation() const {
    const std::lock_guard<std::mutex> lock(mutex_);
    return generation_;
}

bool redirectHeadersAllowed(const std::string& from, const std::string& to) {
    UrlParts fromParts;
    UrlParts toParts;
    if (!validUrl(from, &fromParts) || !validUrl(to, &toParts)) return false;
    return fromParts.scheme == toParts.scheme && fromParts.host == toParts.host &&
           normalizedPort(fromParts) == normalizedPort(toParts);
}

bool redirectTargetAllowed(const std::string& from, const std::string& to) {
    UrlParts fromParts;
    UrlParts toParts;
    if (!validUrl(from, &fromParts) || !validUrl(to, &toParts)) return false;
    return !(fromParts.scheme == "https" && toParts.scheme == "http");
}

std::pair<int, int> mapTouch(int rawX, int rawY, const TouchCalibration& calibration) {
    const int sourceX = calibration.swapAxes ? rawY : rawX;
    const int sourceY = calibration.swapAxes ? rawX : rawY;
    const int minimumX = calibration.swapAxes ? calibration.minY : calibration.minX;
    const int maximumX = calibration.swapAxes ? calibration.maxY : calibration.maxX;
    const int minimumY = calibration.swapAxes ? calibration.minX : calibration.minY;
    const int maximumY = calibration.swapAxes ? calibration.maxX : calibration.maxY;
    const auto mapAxis = [](int raw, int minimum, int maximum, int extent, bool invert) {
        if (maximum <= minimum) {
            minimum = 0;
            maximum = 4095;
        }
        const int clamped = std::max(minimum, std::min(maximum, raw));
        double fraction = static_cast<double>(clamped - minimum) / static_cast<double>(maximum - minimum);
        if (invert) fraction = 1.0 - fraction;
        return static_cast<int>(std::lround(fraction * static_cast<double>(extent - 1)));
    };
    return {mapAxis(sourceX, minimumX, maximumX, 240, calibration.invertX),
            mapAxis(sourceY, minimumY, maximumY, 320, calibration.invertY)};
}

}  // namespace deskduo
