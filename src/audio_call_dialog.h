#pragma once

namespace audio_call_dialog {
enum class Stage { Confirm, Incoming, Active, Done };
enum class Action { None, Call, Cancel, Accept, Decline, Mute, HangUp, Close };

struct Layout {
    static constexpr int left = 184, top = 55, width = 592, height = 434;
    static constexpr int center_x = left + width / 2;
    static constexpr int button_y = 367, button_width = 204, button_height = 62, gap = 16;
    Stage stage;

    constexpr int count() const { return stage == Stage::Done ? 1 : 2; }
    constexpr int x(int index) const {
        return center_x - (count() * button_width + (count() - 1) * gap) / 2 + index * (button_width + gap);
    }
    constexpr Action action(int index) const {
        if (index < 0 || index >= count()) return Action::None;
        if (stage == Stage::Done) return Action::Close;
        if (stage == Stage::Confirm) return index ? Action::Call : Action::Cancel;
        if (stage == Stage::Incoming) return index ? Action::Accept : Action::Decline;
        return index ? Action::HangUp : Action::Mute;
    }
    constexpr int hit(int px, int py) const {
        if (py < button_y || py >= button_y + button_height) return -1;
        for (int i = 0; i < count(); ++i)
            if (px >= x(i) && px < x(i) + button_width) return i;
        return -1;
    }
};

struct Card {
    int x, y, width, height;
    constexpr bool hit(int px, int py) const {
        return px >= x && px < x + width && py >= y && py < y + height;
    }
};
}
