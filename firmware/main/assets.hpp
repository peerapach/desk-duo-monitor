#pragma once
#include <cmath>
#include <cstdio>
#include <string>
#include <vector>
namespace deskduo {
extern const char portal_html[];
extern const char* const sprite_claude[16];
extern const char* const sprite_codex[16];
inline std::string compactNumber(double value) {
    char buffer[24];
    if (!std::isfinite(value)) return "--";
    if (std::abs(value) >= 1000000 || (value != 0 && std::abs(value) < 0.01))
        std::snprintf(buffer, sizeof(buffer), "%.2g", value);
    else std::snprintf(buffer, sizeof(buffer), "%.2f", value);
    return buffer;
}
// Thai shaping adapted from tamaclaude tools/gen/thai.py and thai_table.py.
// MIT (c) 2026 Uthai Moolpak; see THIRD_PARTY_NOTICES.md. Glyphs are Sarabun/OFL.
inline std::string shapeText(const std::string& text) {
    constexpr unsigned above[] = {0xE31,0xE34,0xE35,0xE36,0xE37,0xE47,0xE4D,0xE4E};
    constexpr unsigned contextualAbove[] = {0xE31,0xE34,0xE36,0xE37,0xE47,0xE4D,0xE4E};
    auto canonical = [&](unsigned cp) {
        if (cp >= 0xF700 && cp <= 0xF707) return above[cp - 0xF700];
        if (cp >= 0xF708 && cp <= 0xF716) return 0xE48U + (cp - 0xF708) % 5;
        if (cp >= 0xF717 && cp <= 0xF719) return 0xE38U + cp - 0xF717;
        if (cp >= 0xF720 && cp <= 0xF765) return 0xE48U + (cp - 0xF720) % 5;
        if (cp == 0xF71A) return 0xE0DU;
        if (cp == 0xF71B) return 0xE10U;
        return cp;
    };
    auto kind = [&](unsigned cp) {
        cp = canonical(cp);
        for (unsigned a : above) if (cp == a) return 1;
        if (cp >= 0xE48 && cp <= 0xE4C) return 2;
        if (cp >= 0xE38 && cp <= 0xE3A) return 3;
        return 0;
    };
    std::vector<unsigned> cps;
    for (std::size_t i = 0; i < text.size();) {
        unsigned cp = static_cast<unsigned char>(text[i++]);
        unsigned count = cp < 128 ? 0 : cp < 224 ? 1 : cp < 240 ? 2 : 3;
        if (count) cp &= (1U << (6 - count)) - 1;
        while (count-- && i < text.size()) cp = (cp << 6) | (static_cast<unsigned char>(text[i++]) & 63);
        // Restore context carried by alternate glyphs before shaping again.
        // This also restores the above dot when a merged E4D+tone was emitted.
        if (cp >= 0xF720 && cp <= 0xF765) {
            cps.push_back(contextualAbove[(cp - 0xF720) / 10]);
            cps.push_back(canonical(cp));
        } else cps.push_back(cp == 0xF71A || cp == 0xF71B ? canonical(cp) : cp);
    }
    std::string out;
    auto emit = [&](unsigned cp) {
        if (!((cp >= 32 && cp <= 126) || cp == '\n' || cp == 0xB0 ||
              (cp >= 0xE01 && cp <= 0xE3A) || (cp >= 0xE3F && cp <= 0xE5B) ||
              (cp >= 0xF700 && cp <= 0xF71B) || (cp >= 0xF720 && cp <= 0xF765))) cp = '?';
        if (cp < 128) out += static_cast<char>(cp);
        else if (cp < 2048) { out += static_cast<char>(0xC0 | (cp >> 6)); out += static_cast<char>(0x80 | (cp & 63)); }
        else { out += static_cast<char>(0xE0 | (cp >> 12)); out += static_cast<char>(0x80 | ((cp >> 6) & 63)); out += static_cast<char>(0x80 | (cp & 63)); }
    };
    for (std::size_t i = 0; i < cps.size();) {
        unsigned base = cps[i++];
        if (kind(base)) continue;
        unsigned marks[4]{};
        while (i < cps.size() && kind(cps[i])) {
            unsigned k = kind(cps[i]);
            if (!marks[k]) marks[k] = canonical(cps[i]);
            ++i;
        }
        bool tall = base == 0xE1B || base == 0xE1D || base == 0xE1F || base == 0xE2C;
        bool desc = base == 0xE0D || base == 0xE10;
        emit(desc && marks[3] ? (base == 0xE0D ? 0xF71A : 0xF71B) : base);
        if (marks[3]) emit(desc ? 0xF717 + marks[3] - 0xE38 : marks[3]);
        bool mergedDotTone = marks[1] == 0xE4D && marks[2] >= 0xE48 && marks[2] <= 0xE4B;
        if (marks[1] && !mergedDotTone) {
            unsigned cp = marks[1];
            if (tall) for (unsigned k = 0; k < 8; ++k) if (cp == above[k]) { cp = 0xF700 + k; break; }
            emit(cp);
        }
        if (marks[2]) {
            unsigned start = marks[1] ? (tall ? 0xF712 : 0xF70D) : (tall ? 0xF708 : 0xE48);
            if (marks[1] && marks[1] != 0xE35) {
                for (unsigned k = 0; k < 7; ++k) if (marks[1] == contextualAbove[k]) {
                    start = 0xF720 + k * 10 + (tall ? 5 : 0);
                    break;
                }
            }
            emit(start + marks[2] - 0xE48);
        }
    }
    return out;
}
}
