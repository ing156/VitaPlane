#pragma once
#include "reaction_input.h"

struct StickerGrid {
    static constexpr int left=30, top=130, columns=6, page_size=18;
    static constexpr int step_x=150, step_y=110, width=144, height=104;
    static int x(int cell) { return left + cell%columns*step_x; }
    static int y(int cell) { return top + cell/columns*step_y; }
    static int hit(int px, int py, int current, int count) {
        px-=left; py-=top;
        if (px<0 || py<0 || px>=columns*step_x || py>=page_size/columns*step_y || px%step_x>=width || py%step_y>=height) return -1;
        const int index=current/page_size*page_size+py/step_y*columns+px/step_x;
        return index<count ? index : -1;
    }
};

class PickerPress {
public:
    struct Target {
        int64_t chat = 0;
        int topic = 0, pack = 0, index = -1, file = 0;
        bool operator==(const Target& other) const {
            return chat == other.chat && topic == other.topic && pack == other.pack &&
                index == other.index && file == other.file;
        }
    };
    struct Result { bool send, consume_cross, consume_touch, preview; };
    Result update(bool context, Target target, bool cross_down, bool cross_fresh,
                  bool touching, bool touch_fresh, int touched_index, bool cancel, int64_t now) {
        const bool changed = !(target == target_);
        target_ = target;
        const auto cross = cross_.update(context, cross_down, cross_fresh,
            cancel || touching || (changed && !cross_fresh), target.chat, target.index,
            target.topic, now);
        const auto touch = touch_.update(context, touching, touch_fresh && touched_index >= 0,
            cancel || cross_down || touched_index != target.index || (changed && !touch_fresh),
            target.chat, target.index, target.topic, now);
        if (!context || cancel || changed || (cross_down && touching) || (!cross_down && !touching) ||
            (touching && touched_index != target.index)) preview_ = false;
        if ((cross.long_press && cross_down) || (touch.long_press && touching)) preview_ = true;
        return {cross.short_press || touch.short_press, cross.consume, touch.consume, preview_};
    }
private:
    Target target_;
    ReactionPress cross_, touch_;
    bool preview_ = false;
};
