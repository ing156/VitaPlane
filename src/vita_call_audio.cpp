#include "vita_call_audio.h"
#include "cpu_scheduler.h"

#include <algorithm>
#include <array>
#include <system_error>
#include <thread>

#include <psp2/audioin.h>
#include <psp2/audioout.h>
#include <psp2/kernel/threadmgr.h>

#include "modules/audio_device/audio_device_buffer.h"
#include "modules/audio_device/fine_audio_buffer.h"
#include "modules/audio_device/include/audio_device_default.h"
#include "rtc_base/ref_counted_object.h"

namespace {
constexpr int kCaptureFrames = 256; // 16 ms in the native voice capture profile.
// Keep the extra 20 ms of headroom in the stable mode.
constexpr int kLowLatencyPlaybackFrames = 960;
constexpr int kStablePlaybackFrames = 2 * kLowLatencyPlaybackFrames;
constexpr int kCaptureRate = 16000;
constexpr int kPlaybackRate = 48000;
static_assert(kLowLatencyPlaybackFrames % SCE_AUDIO_MIN_LEN == 0);
static_assert(kLowLatencyPlaybackFrames % (kPlaybackRate / 100) == 0);

void observe_gap(std::atomic<unsigned>& maximum_ms, std::atomic<unsigned>& late,
                 unsigned period_ms, uint64_t& previous) {
    const auto now = sceKernelGetSystemTimeWide();
    if (previous) {
        const auto elapsed = now - previous;
        const auto gap = unsigned((elapsed + 500) / 1000);
        if (gap > maximum_ms.load(std::memory_order_relaxed))
            maximum_ms.store(gap, std::memory_order_relaxed);
        if (elapsed >= period_ms * 1500)
            late.fetch_add(1, std::memory_order_relaxed);
    }
    previous = now;
}

class VitaAudioDevice : public webrtc::webrtc_impl::AudioDeviceModuleDefault<webrtc::AudioDeviceModule> {
public:
    VitaAudioDevice(webrtc::TaskQueueFactory* factory, VitaAudioStats& stats, bool low_latency)
        : buffer_(factory), stats_(stats),
          playback_frames_(low_latency ? kLowLatencyPlaybackFrames : kStablePlaybackFrames) {
        buffer_.SetRecordingSampleRate(kCaptureRate);
        buffer_.SetRecordingChannels(1);
        buffer_.SetPlayoutSampleRate(kPlaybackRate);
        buffer_.SetPlayoutChannels(1);
    }
    ~VitaAudioDevice() override { Terminate(); }
    int32_t RegisterAudioCallback(webrtc::AudioTransport* callback) override {
        return buffer_.RegisterAudioCallback(callback);
    }
    int32_t Init() override { initialized_ = true; return 0; }
    bool Initialized() const override { return initialized_; }
    int32_t Terminate() override {
        StopRecording();
        StopPlayout();
        initialized_ = false;
        return 0;
    }
    int32_t RecordingIsAvailable(bool* available) override { *available = true; return 0; }
    int32_t PlayoutIsAvailable(bool* available) override { *available = true; return 0; }
    int32_t SetStereoPlayout(bool enabled) override { return enabled ? -1 : 0; }
    int32_t SetStereoRecording(bool enabled) override { return enabled ? -1 : 0; }
    int32_t StereoPlayout(bool* enabled) const override { *enabled = false; return 0; }
    int32_t StereoRecording(bool* enabled) const override { *enabled = false; return 0; }
    bool RecordingIsInitialized() const override { return capture_port_ >= 0; }
    bool PlayoutIsInitialized() const override { return playback_port_ >= 0; }
    bool Recording() const override { return capturing_; }
    bool Playing() const override { return playing_; }
    int32_t InitRecording() override {
        if (!initialized_) return -1;
        if (capture_port_ < 0) {
            capture_port_ = sceAudioInOpenPort(SCE_AUDIO_IN_PORT_TYPE_VOICE, kCaptureFrames, kCaptureRate,
                                               SCE_AUDIO_IN_PARAM_FORMAT_S16_MONO);
        }
        if (capture_port_ < 0) stats_.read_result = capture_port_;
        return capture_port_ < 0 ? capture_port_ : 0;
    }
    int32_t InitPlayout() override {
        if (!initialized_) return -1;
        if (playback_port_ < 0) {
            playback_port_ = sceAudioOutOpenPort(SCE_AUDIO_OUT_PORT_TYPE_MAIN, playback_frames_, kPlaybackRate,
                                                 SCE_AUDIO_OUT_MODE_MONO);
        }
        if (playback_port_ < 0) stats_.write_result = playback_port_;
        return playback_port_ < 0 ? playback_port_ : 0;
    }
    int32_t StartRecording() override {
        if (capturing_) return 0;
        if (capture_port_ < 0 || capture_thread_.joinable()) return -1;
        buffer_.StartRecording();
        capturing_ = true;
        try {
            capture_thread_ = std::thread([this, port = capture_port_] {
                // Leave CPU1 to playout so its decoding cannot preempt capture.
                cpu_scheduler::Thread scheduling(cpu_scheduler::Role::CallCapture);
                webrtc::FineAudioBuffer capture(&buffer_);
                alignas(64) std::array<int16_t, kCaptureFrames> pcm{};
                uint64_t previous = 0;
                while (capturing_) {
                    const auto result = sceAudioInInput(port, pcm.data());
                    stats_.read_result = result;
                    if (result < 0 || !capturing_) break;
                    stats_.capture_blocks.fetch_add(1, std::memory_order_relaxed);
                    observe_gap(stats_.capture_gap_ms, stats_.capture_late, kCaptureFrames * 1000 / kCaptureRate, previous);
                    unsigned peak = 0, clipped = 0;
                    for (int sample : pcm) {
                        const unsigned magnitude = sample < 0 ? -sample : sample;
                        peak = std::max(peak, magnitude);
                        clipped += sample == INT16_MIN || sample == INT16_MAX;
                    }
                    if (clipped) stats_.clipped_samples.fetch_add(clipped, std::memory_order_relaxed);
                    if (peak > stats_.capture_peak.load(std::memory_order_relaxed))
                        stats_.capture_peak.store(peak, std::memory_order_relaxed);
                    capture.SetPlayoutDelay(playout_delay_ms_.load(std::memory_order_relaxed));
                    capture.DeliverRecordedData(pcm, kCaptureFrames * 1000 / kCaptureRate);
                }
                capturing_ = false;
            });
        } catch (const std::system_error& error) {
            StopRecording();
            stats_.read_result = -error.code().value();
            return stats_.read_result;
        }
        return 0;
    }
    int32_t StartPlayout() override {
        if (playing_) return 0;
        if (playback_port_ < 0 || playback_thread_.joinable()) return -1;
        buffer_.StartPlayout();
        playing_ = true;
        try {
            playback_thread_ = std::thread([this, port = playback_port_] {
                cpu_scheduler::Thread scheduling(cpu_scheduler::Role::Audio);
                webrtc::FineAudioBuffer playback(&buffer_);
                alignas(64) std::array<std::array<int16_t, kStablePlaybackFrames>, 2> pcm{};
                unsigned slot = 0;
                uint64_t previous = 0;
                while (playing_) {
                    const int queued = sceAudioOutGetRestSample(port);
                    const int delay_ms = (std::max(0, queued) + playback_frames_ / 2) * 1000 / kPlaybackRate;
                    playback.GetPlayoutData({pcm[slot].data(), playback_frames_}, delay_ms);
                    if (!playing_) break;
                    const auto result = sceAudioOutOutput(port, pcm[slot].data());
                    stats_.write_result = result;
                    if (result < 0) break;
                    playout_delay_ms_.store(delay_ms, std::memory_order_relaxed);
                    observe_gap(stats_.playout_gap_ms, stats_.playout_late, playback_frames_ * 1000 / kPlaybackRate, previous);
                    slot ^= 1;
                }
                if (previous) sceAudioOutOutput(port, nullptr);
                playout_delay_ms_.store(0, std::memory_order_relaxed);
                playing_ = false;
            });
        } catch (const std::system_error& error) {
            StopPlayout();
            stats_.write_result = -error.code().value();
            return stats_.write_result;
        }
        return 0;
    }
    int32_t StopRecording() override {
        if (capture_port_ < 0) return 0;
        capturing_ = false;
        if (capture_thread_.joinable()) capture_thread_.join();
        buffer_.StopRecording();
        sceAudioInReleasePort(capture_port_);
        capture_port_ = -1;
        return 0;
    }
    int32_t StopPlayout() override {
        if (playback_port_ < 0) return 0;
        playing_ = false;
        if (playback_thread_.joinable()) playback_thread_.join();
        buffer_.StopPlayout();
        sceAudioOutReleasePort(playback_port_);
        playback_port_ = -1;
        return 0;
    }
private:
    webrtc::AudioDeviceBuffer buffer_;
    VitaAudioStats& stats_;
    const size_t playback_frames_;
    std::thread capture_thread_;
    std::thread playback_thread_;
    std::atomic<bool> capturing_{false};
    std::atomic<bool> playing_{false};
    std::atomic<int> playout_delay_ms_{0};
    bool initialized_ = false;
    int capture_port_ = -1;
    int playback_port_ = -1;
};
}

webrtc::scoped_refptr<webrtc::AudioDeviceModule> CreateVitaAudioDevice(
    webrtc::TaskQueueFactory* factory, VitaAudioStats& stats, bool low_latency) {
    return webrtc::scoped_refptr<webrtc::AudioDeviceModule>(
        new rtc::RefCountedObject<VitaAudioDevice>(factory, stats, low_latency));
}
