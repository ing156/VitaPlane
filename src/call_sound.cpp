#include "call_sound.h"
#include "cpu_scheduler.h"

#include <algorithm>
#include <array>
#include <chrono>
#include <cmath>
#include <system_error>

#include <psp2/audioout.h>

namespace {
constexpr int kRate = 48000, kFrames = 960;
constexpr int kRampFrames = kRate / 200;
static_assert(kFrames % SCE_AUDIO_MIN_LEN == 0);

struct Step {
    int ms, low, high;
};
constexpr Step kIncoming[] = {{180, 660, 990}, {100, 0, 0}, {240, 784, 1176}, {1280, 0, 0}};
constexpr Step kOutgoing[] = {{800, 440, 480}, {2400, 0, 0}};

class Oscillator {
public:
    explicit Oscillator(int hz)
        : sine_step_(std::sin(hz * kRadiansPerSample)), cosine_step_(std::cos(hz * kRadiansPerSample)) {}

    double next() {
        const double sample = sine_;
        sine_ = sine_ * cosine_step_ + cosine_ * sine_step_;
        cosine_ = cosine_ * cosine_step_ - sample * sine_step_;
        return sample;
    }

private:
    static constexpr double kRadiansPerSample = 6.283185307179586 / kRate;
    double sine_ = 0, cosine_ = 1;
    const double sine_step_, cosine_step_;
};
}

CallSound::~CallSound() { shutdown(); }

void CallSound::set(Tone tone) {
    std::lock_guard<std::mutex> lock(mutex_);
    if (stopped_ || tone_ == tone) return;
    tone_ = tone;
    ++revision_;
    if (tone != Tone::Silent && !worker_.joinable()) {
        try {
            worker_ = std::thread(&CallSound::run, this);
        } catch (const std::system_error&) {
            return;
        }
    }
    wake_.notify_one();
}

void CallSound::shutdown() {
    {
        std::lock_guard<std::mutex> lock(mutex_);
        stopped_ = true;
    }
    wake_.notify_one();
    if (worker_.joinable()) worker_.join();
}

void CallSound::run() {
    std::unique_lock<std::mutex> lock(mutex_);
    uint64_t observed = 0;
    while (true) {
        wake_.wait(lock, [&] { return stopped_ || revision_ != observed; });
        if (stopped_) return;
        observed = revision_;
        const auto tone = tone_;
        lock.unlock();
        if (tone != Tone::Silent) play(tone, observed);
        lock.lock();
    }
}

void CallSound::play(Tone tone, uint64_t revision) {
    const auto changed = [&] { return stopped_ || revision_ != revision; };
    const int port = sceAudioOutOpenPort(SCE_AUDIO_OUT_PORT_TYPE_MAIN, kFrames, kRate, SCE_AUDIO_OUT_MODE_MONO);
    if (port < 0) return;
    cpu_scheduler::Thread scheduling(cpu_scheduler::Role::Audio);
    const bool incoming = tone == Tone::Incoming;
    const auto* steps = incoming ? kIncoming : kOutgoing;
    const size_t count = incoming ? std::size(kIncoming) : std::size(kOutgoing);
    alignas(64) std::array<std::array<int16_t, kFrames>, 2> pcm{};
    unsigned slot = 0;
    bool submitted = false, finished = false;
    while (!finished) {
        for (size_t i = 0; i < count && !finished; ++i) {
            const auto& step = steps[i];
            if (!step.low) {
                std::unique_lock<std::mutex> lock(mutex_);
                finished = wake_.wait_for(lock, std::chrono::milliseconds(step.ms), changed);
                continue;
            }
            Oscillator low(step.low), high(step.high);
            const int frames = step.ms * kRate / 1000;
            for (int block = 0; block < frames; block += kFrames) {
                {
                    std::lock_guard<std::mutex> lock(mutex_);
                    if (changed()) { finished = true; break; }
                }
                for (int i = 0; i < kFrames; ++i) {
                    const int frame = block + i;
                    const double gain = std::min({kRampFrames, frame, frames - 1 - frame}) / double(kRampFrames);
                    pcm[slot][i] = int16_t(2400 * gain * (low.next() + high.next()));
                }
                if (sceAudioOutOutput(port, pcm[slot].data()) < 0) { finished = true; break; }
                submitted = true;
                slot ^= 1;
            }
        }
    }
    if (submitted) sceAudioOutOutput(port, nullptr);
    sceAudioOutReleasePort(port);
}
