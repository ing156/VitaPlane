#pragma once

#include <atomic>

#include "api/scoped_refptr.h"
#include "modules/audio_device/include/audio_device.h"

struct VitaAudioStats {
    std::atomic<int> read_result{0};
    std::atomic<int> write_result{0};
    std::atomic<unsigned> capture_gap_ms{0}, playout_gap_ms{0};
    std::atomic<unsigned> capture_late{0}, playout_late{0};
    std::atomic<unsigned> capture_blocks{0};
    std::atomic<unsigned> clipped_samples{0}, capture_peak{0};
};

webrtc::scoped_refptr<webrtc::AudioDeviceModule> CreateVitaAudioDevice(
    webrtc::TaskQueueFactory* factory, VitaAudioStats& stats, bool low_latency);
