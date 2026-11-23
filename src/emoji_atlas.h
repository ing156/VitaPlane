#pragma once
#include <algorithm>
#include <cstddef>
#include <cstdint>
#include <cstring>
#include <string>
#include <vector>

// One shared CDRAM page for inline custom emoji. vita2d rounds every texture up to 256 KiB, so a texture
// per emoji capped a whole screen at ~128 images and dense posts evicted each other every frame.
struct EmojiAtlas {
    static constexpr int cell = 48, grid = 21, side = cell * grid, content = cell - 2; // 1px transparent gutter
    struct Slot { std::string path; unsigned used = 0; int w = 0, h = 0; };
    std::vector<Slot> slots = std::vector<Slot>(grid * grid);

    int find(const std::string& path) const {
        for (size_t i = 0; i < slots.size(); ++i) if (!path.empty() && slots[i].path == path) return (int)i;
        return -1;
    }
    // Writes the image, fitted into one cell, into the atlas pixels; reuses path's slot, else the least recently used.
    // shortcut: more than grid*grid distinct emoji on screen evict each other again, raise grid if that shows up.
    int store(const std::string& path, const uint8_t* rgba, unsigned w, unsigned h, uint8_t* pixels, size_t stride, unsigned now) {
        if (!w || !h || !rgba || !pixels) return -1;
        int index = find(path);
        if (index < 0) index = (int)(std::min_element(slots.begin(), slots.end(),
            [](const Slot& a, const Slot& b) { return a.used < b.used; }) - slots.begin());
        const unsigned longest = std::max(w, h);
        const unsigned dw = longest > (unsigned)content ? std::max(1u, w * content / longest) : w;
        const unsigned dh = longest > (unsigned)content ? std::max(1u, h * content / longest) : h;
        const int cx = index % grid * cell, cy = index / grid * cell;
        for (int y = 0; y < cell; ++y) std::memset(pixels + (size_t)(cy + y) * stride + (size_t)cx * 4, 0, cell * 4);
        const int ox = cx + 1 + (content - (int)dw) / 2, oy = cy + 1 + (content - (int)dh) / 2;
        for (unsigned y = 0; y < dh; ++y) for (unsigned x = 0; x < dw; ++x) {
            const unsigned x0 = x * w / dw, x1 = std::max(x0 + 1, (x + 1) * w / dw);
            const unsigned y0 = y * h / dh, y1 = std::max(y0 + 1, (y + 1) * h / dh);
            uint64_t sum[4] = {0, 0, 0, 0}, count = 0;
            for (unsigned sy = y0; sy < y1; ++sy) for (unsigned sx = x0; sx < x1; ++sx) {
                const uint8_t* p = rgba + ((size_t)sy * w + sx) * 4;
                for (int c = 0; c < 3; ++c) sum[c] += (uint64_t)p[c] * p[3]; // alpha-weighted: no dark fringe
                sum[3] += p[3]; ++count;
            }
            uint8_t* out = pixels + (size_t)(oy + y) * stride + (size_t)(ox + x) * 4;
            for (int c = 0; c < 3; ++c) out[c] = sum[3] ? (uint8_t)(sum[c] / sum[3]) : 0;
            out[3] = (uint8_t)(sum[3] / count);
        }
        slots[index] = {path, now, (int)dw, (int)dh};
        return index;
    }
    void forget(const std::string& path) { const int i = find(path); if (i >= 0) slots[i] = {}; }
    void clear() { slots.assign(grid * grid, {}); }
};
