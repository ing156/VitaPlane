#include "call_sound.h"
#include "cpu_scheduler.h"

#include <algorithm>
#include <array>
#include <cassert>
#include <chrono>
#include <condition_variable>
#include <mutex>
#include <thread>
#include <vector>

#include <psp2/audioout.h>

namespace {
using Pcm = std::array<int16_t, 960>;
std::mutex mutex;
std::condition_variable updated;
std::vector<Pcm> first_buffers;
unsigned closed = 0, blocks = 0;
bool first = false;

template <typename Predicate>
void wait_for(Predicate ready) {
    std::unique_lock<std::mutex> lock(mutex);
    assert(updated.wait_for(lock, std::chrono::seconds(2), ready));
}
}

cpu_scheduler::Thread::Thread(Role) : id_(0) {}
cpu_scheduler::Thread::~Thread() = default;

int sceAudioOutOpenPort(SceAudioOutPortType type, int frames, int rate, SceAudioOutMode mode) {
    assert(type == SCE_AUDIO_OUT_PORT_TYPE_MAIN && frames == 960 && rate == 48000 && mode == SCE_AUDIO_OUT_MODE_MONO);
    std::lock_guard<std::mutex> lock(mutex);
    first = true;
    return 7;
}

int sceAudioOutOutput(int port, const void* audio) {
    assert(port == 7);
    if (!audio) return 0;
    {
        std::lock_guard<std::mutex> lock(mutex);
        ++blocks;
        if (first) {
            Pcm pcm;
            std::copy_n(static_cast<const int16_t*>(audio), pcm.size(), pcm.begin());
            first_buffers.push_back(pcm);
            first = false;
        }
    }
    updated.notify_one();
    std::this_thread::sleep_for(std::chrono::milliseconds(2));
    return 960;
}

int sceAudioOutReleasePort(int port) {
    assert(port == 7);
    {
        std::lock_guard<std::mutex> lock(mutex);
        ++closed;
    }
    updated.notify_one();
    return 0;
}

int main() {
    using Tone = CallSound::Tone;
    CallSound sound;
    sound.set(Tone::Silent);
    sound.set(Tone::Incoming);
    wait_for([] { return first_buffers.size() == 1 && blocks >= 3; });
    sound.set(Tone::Incoming);
    sound.set(Tone::Silent);
    wait_for([] { return closed == 1; });
    sound.set(Tone::Outgoing);
    wait_for([] { return first_buffers.size() == 2; });
    sound.shutdown();
    assert(closed == 2);
    assert(first_buffers[0] != first_buffers[1]);
    for (const auto& pcm : first_buffers) {
        assert(pcm.front() == 0);
        assert(std::any_of(pcm.begin(), pcm.end(), [](int16_t sample) { return sample != 0; }));
        assert(std::all_of(pcm.begin(), pcm.end(), [](int16_t sample) { return sample >= -4800 && sample <= 4800; }));
    }
}
