#pragma once
#include <algorithm>
#include <functional>
#include <cstdint>
#include <string>
#include <vector>

using CustomEmojiDraw = std::function<bool(int64_t, float, float, float, unsigned)>;

struct CustomEmojiSpan { int begin = 0, end = 0; int64_t id = 0; };

// Byte ranges stay attached to the inserted emoji, even beside identical symbols.
inline void shift_custom_emojis(std::vector<CustomEmojiSpan>& spans, int begin, int end, int added) {
    spans.erase(std::remove_if(spans.begin(), spans.end(), [&](CustomEmojiSpan& span) {
        if (span.end <= begin) return false;
        if (span.begin < end) return true;
        span.begin += added - (end-begin); span.end += added - (end-begin);
        return false;
    }), spans.end());
}
inline int custom_emoji_utf16(const std::string& text, int end) {
    if (end < 0 || end > (int)text.size()) return -1;
    int units = 0;
    for (int i = 0; i < end;) {
        const auto c = (unsigned char)text[i];
        const int n = c < 0x80 ? 1 : c >= 0xf0 && c <= 0xf4 ? 4 : c >= 0xe0 && c < 0xf0 ? 3 : c >= 0xc2 && c < 0xe0 ? 2 : 0;
        if (!n || i+n > end) return -1;
        for (int j=1; j<n; ++j) if (((unsigned char)text[i+j]&0xc0) != 0x80) return -1;
        units += n == 4 ? 2 : 1; i += n;
    }
    return units;
}
