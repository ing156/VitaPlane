#pragma once

#include <condition_variable>
#include <cstdint>
#include <mutex>
#include <thread>

class CallSound {
public:
    enum class Tone { Silent, Incoming, Outgoing };
    ~CallSound();
    void set(Tone tone);
    void shutdown();

private:
    void run();
    void play(Tone tone, uint64_t revision);
    std::mutex mutex_;
    std::condition_variable wake_;
    std::thread worker_;
    Tone tone_ = Tone::Silent;
    uint64_t revision_ = 0;
    bool stopped_ = false;
};
