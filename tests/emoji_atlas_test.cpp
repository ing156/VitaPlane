#include "emoji_atlas.h"
#include <cassert>
#include <cstdio>

int main() {
    std::vector<uint8_t> page((size_t)EmojiAtlas::side * EmojiAtlas::side * 4, 0xaa);
    const size_t stride = (size_t)EmojiAtlas::side * 4;
    EmojiAtlas atlas;
    // 128x128 half-transparent source: left half red opaque, right half fully transparent black.
    std::vector<uint8_t> src(128 * 128 * 4, 0);
    for (int y = 0; y < 128; ++y) for (int x = 0; x < 64; ++x) { auto* p = &src[(y * 128 + x) * 4]; p[0] = 255; p[3] = 255; }
    unsigned clock = 0;
    // 200 distinct emoji (above the old ~128 texture ceiling) all keep their own cell.
    for (int i = 0; i < 200; ++i) assert(atlas.store("e" + std::to_string(i), src.data(), 128, 128, page.data(), stride, ++clock) == i);
    for (int i = 0; i < 200; ++i) assert(atlas.find("e" + std::to_string(i)) == i);
    const auto& s = atlas.slots[0];
    assert(s.w == EmojiAtlas::content && s.h == EmojiAtlas::content);
    // Gutter cleared, edge of the red half keeps full red (alpha-weighted, no dark fringe).
    assert(page[3] == 0 && page[0] == 0);
    const uint8_t* edge = &page[(size_t)(1 + 10) * stride + (size_t)(1 + 22) * 4];
    assert(edge[0] == 255 && edge[3] > 0);
    // Re-storing the same path reuses its slot; a full atlas evicts the least recently used one.
    assert(atlas.store("e5", src.data(), 128, 128, page.data(), stride, ++clock) == 5);
    for (int i = 200; i < EmojiAtlas::grid * EmojiAtlas::grid; ++i) atlas.store("e" + std::to_string(i), src.data(), 128, 128, page.data(), stride, ++clock);
    atlas.slots[0].used = ++clock; // e0 drawn again this frame
    assert(atlas.store("new", src.data(), 128, 128, page.data(), stride, ++clock) == 1);
    // Small images are centered, not stretched; forget frees the cell.
    const int small = atlas.store("tiny", src.data(), 128, 64, page.data(), stride, ++clock);
    assert(atlas.slots[small].w == EmojiAtlas::content && atlas.slots[small].h == EmojiAtlas::content / 2);
    atlas.forget("tiny"); assert(atlas.find("tiny") < 0);
    assert(atlas.store("bad", nullptr, 0, 0, page.data(), stride, ++clock) < 0);
    std::puts("emoji atlas: ok");
}
