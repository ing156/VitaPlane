#pragma once
#include <map>
#include <string>
#include <cstdint>
#include <tuple>

// UI-owned cooldowns: failed textures must not become permanent cache entries,
// but a damaged file must not be decoded on every frame either.
class ImageRetry {
    struct Entry { unsigned failures = 0; int64_t next = 0; };
    std::map<std::tuple<std::string, unsigned, int>, Entry> entries_;
public:
    bool ready(const std::string& path, int64_t now, unsigned max_side = 1024, int mask = 0) const {
        auto it = entries_.find({path,max_side,mask});
        return it == entries_.end() || now >= it->second.next;
    }
    unsigned fail(const std::string& path, int64_t now, unsigned max_side = 1024, int mask = 0) {
        if (!entries_.count({path,max_side,mask}) && entries_.size() >= 256) entries_.erase(entries_.begin());
        auto& entry = entries_[{path,max_side,mask}];
        if (entry.failures < 4) ++entry.failures;
        const int64_t delays[] = {1000000, 3000000, 10000000, 30000000};
        entry.next = now + delays[entry.failures - 1];
        return entry.failures;
    }
    void forget(const std::string& path, unsigned max_side, int mask) { entries_.erase({path,max_side,mask}); }
    void forget(const std::string& path) {
        for (auto it=entries_.begin();it!=entries_.end();)
            if (std::get<0>(it->first)==path) it=entries_.erase(it); else ++it;
    }
    void clear() { entries_.clear(); }
};
