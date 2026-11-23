#pragma once
#include "emoji_data.h"
#include "utf8_decode.h"
#include <algorithm>
#include <iterator>
#include <string_view>

namespace emoji {
using emoji_data::Entry;
inline const Entry* find(std::string_view text) {
    const auto* end = std::end(emoji_data::entries);
    const auto* entry = std::lower_bound(std::begin(emoji_data::entries), end, text,
        [](const Entry& a, std::string_view b) { return std::string_view(a.text) < b; });
    return entry != end && entry->text == text ? entry : nullptr;
}
template<size_t N> inline const emoji_data::Range* range(uint32_t code, const emoji_data::Range (&ranges)[N]) {
    auto* end = std::end(ranges);
    auto* found = std::lower_bound(std::begin(ranges), end, code,
        [](const emoji_data::Range& a, uint32_t b) { return a.last < b; });
    return found != end && found->first <= code ? found : nullptr;
}
inline unsigned property(uint32_t code) {
    const auto* found = range(code, emoji_data::breaks);
    return found ? unsigned(found->property) : unsigned(emoji_data::Other);
}
inline size_t scalar(std::string_view text, size_t at, uint32_t& code) {
    char bytes[5]{};
    const size_t count = std::min<size_t>(4, text.size() - at);
    std::copy_n(text.data() + at, count, bytes);
    return std::max<size_t>(1, decode_utf8_scalar(bytes, code));
}
// Unicode grapheme boundaries for emoji, combining marks, flags and Hangul.
// shortcut: Indic conjunct GB9c is not implemented; add InCB data when expanding Indic editing support.
inline size_t next(std::string_view text, size_t at) {
    using namespace emoji_data;
    if (at >= text.size()) return text.size();
    uint32_t code = 0;
    size_t end = at + scalar(text, at, code);
    unsigned previous = property(code), regional = previous == Regional_Indicator;
    bool pictograph = range(code, pictographs), joined_pictograph = false;
    while (end < text.size()) {
        uint32_t current = 0;
        const size_t bytes = scalar(text, end, current);
        const unsigned prop = property(current);
        const bool current_pictograph = range(current, pictographs);
        bool join = previous == CR && prop == LF;
        const auto control = [](unsigned p) { return p == CR || p == LF || p == Control; };
        if (!control(previous) && !control(prop)) {
            join = (previous == L && (prop == L || prop == V || prop == LV || prop == LVT)) ||
                ((previous == LV || previous == V) && (prop == V || prop == T)) ||
                ((previous == LVT || previous == T) && prop == T) ||
                prop == Extend || prop == ZWJ || prop == SpacingMark || previous == Prepend ||
                (previous == ZWJ && joined_pictograph && current_pictograph) ||
                (previous == Regional_Indicator && prop == Regional_Indicator && regional % 2 == 1);
        }
        if (!join) break;
        joined_pictograph = prop == ZWJ && pictograph;
        if (prop != Extend) pictograph = current_pictograph;
        regional = prop == Regional_Indicator ? regional + 1 : 0;
        previous = prop;
        end += bytes;
    }
    return end;
}
inline size_t previous(std::string_view text, size_t at) {
    size_t before = 0;
    for (size_t end = 0; end < at;) { before = end; end = next(text, end); }
    return before;
}
}
