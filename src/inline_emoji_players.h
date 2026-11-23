#pragma once
#include <algorithm>
#include <array>
#include <cstdint>
#include <memory>
#include <string>
#include <vector>
#include <utility>

template<class Player> class InlineEmojiPlayers {
public:
    static constexpr size_t capacity=4;
    void beginFrame(int64_t context, bool paused) {
        context_=context; paused_=paused; wanted_.clear();
    }
    bool request(int64_t id, const std::string& path) {
        if (!context_ || !id) return false;
        for (auto& item:wanted_) if (item.id==id) { item.path=path; return true; }
        if (wanted_.size()==capacity) return false;
        wanted_.push_back({id,path}); return true;
    }
    auto texture(int64_t id) -> decltype(std::declval<Player&>().texture()) {
        for (auto& slot:slots_) if (slot.id==id && slot.context==context_ && slot.player &&
            std::any_of(wanted_.begin(),wanted_.end(),[&](const Wanted& item){return item.id==id && item.path==slot.path;})) return slot.player->texture();
        return nullptr;
    }
    std::vector<int64_t> wantedIds() const {
        std::vector<int64_t> result;
        if (paused_) return result;
        for (const auto& item:wanted_) result.push_back(item.id);
        return result;
    }
    void finishFrame() {
        for (auto& slot:slots_) if (slot.id && (slot.context!=context_ ||
            std::none_of(wanted_.begin(),wanted_.end(),[&](const Wanted& item){return item.id==slot.id && item.path==slot.path;}))) {
            slot.player->setSource(""); slot.id=0; slot.path.clear();
        }
        for (const auto& item:wanted_) {
            if (paused_ || item.path.empty() || std::any_of(slots_.begin(),slots_.end(),[&](const Slot& slot){return slot.id==item.id && slot.context==context_;})) continue;
            auto free=std::find_if(slots_.begin(),slots_.end(),[](const Slot& slot){return !slot.id;});
            if (free==slots_.end()) break;
            if (!free->player) free->player=std::make_unique<Player>(true);
            free->id=item.id; free->context=context_; free->path=item.path;
            free->player->setSource(item.path);
        }
        for (auto& slot:slots_) if (slot.id) {
            slot.player->setSuspended(paused_);
            if (!paused_) slot.player->upload();
        }
    }
    void clear() {
        beginFrame(0,true); finishFrame();
    }
    void shutdown() {
        clear(); for (auto& slot:slots_) slot.player.reset();
    }
private:
    struct Wanted { int64_t id; std::string path; };
    struct Slot { int64_t id=0, context=0; std::string path; std::unique_ptr<Player> player; };
    std::array<Slot,capacity> slots_;
    std::vector<Wanted> wanted_;
    int64_t context_=0; bool paused_=true;
};
