#pragma once

#include <psp2/touch.h>
#include <cstdint>
#include <cstdlib>

namespace video_note {
class RearDoubleTap {
public:
    bool update(const SceTouchData& touch, int64_t now) {
        if (touch.reportNum == 0) {
            const bool tap = down_ && !blocked_ && now - down_at_ <= 180000;
            down_ = blocked_ = false;
            if (!tap) return false;
            const bool pair = last_tap_ && down_at_ - last_tap_ <= 350000 &&
                near(x_, y_, last_x_, last_y_);
            last_tap_ = pair ? 0 : now;
            last_x_ = x_; last_y_ = y_;
            return pair;
        }
        if (blocked_) return false;
        const auto& point = touch.report[0];
        if (touch.reportNum != 1 || (down_ && (point.id != id_ || !near(x_, y_, point.x, point.y)))) {
            blocked_ = true;
            last_tap_ = 0;
        } else if (!down_) {
            down_ = true;
            down_at_ = now;
            x_ = point.x; y_ = point.y; id_ = point.id;
        }
        return false;
    }

private:
    static bool near(int x, int y, int other_x, int other_y) {
        return std::abs(x - other_x) <= 120 && std::abs(y - other_y) <= 120;
    }

    bool down_ = false, blocked_ = true;
    int x_ = 0, y_ = 0, last_x_ = 0, last_y_ = 0;
    unsigned id_ = 0;
    int64_t down_at_ = 0, last_tap_ = 0;
};
}
