#include "performance.h"
#include "i18n.h"
#include "qr_scanner.h"
#include "proxy_link.h"
#include "cpu_scheduler.h"
#include <quirc.h>
#include <vita2d.h>
#include <psp2/camera.h>
#include <psp2/kernel/sysmem.h>
#include <psp2/kernel/threadmgr.h>
#include <algorithm>
#include <chrono>
#include <cstdio>
#include <cstring>

namespace {
constexpr int Width = 640, Height = 480, PreviewWidth = 320, PreviewHeight = 240;
constexpr int Device = SCE_CAMERA_DEVICE_BACK;
}
QrScanner::QrScanner() { pthread_mutex_init(&mutex_, nullptr); }
QrScanner::~QrScanner() { stop(); pthread_mutex_destroy(&mutex_); }

bool QrScanner::fail(const i18n::Text& operation, int code) {
    char message[160];
    std::snprintf(message, sizeof(message), ": 0x%08X", (unsigned)code);
    stop();
    pthread_mutex_lock(&mutex_); status_ = operation + message; pthread_mutex_unlock(&mutex_);
    return false;
}

bool QrScanner::start() {
    stop();
    stopping_ = false;
    link_.clear(); preview_ready_ = false;
    status_ = i18n::text("qr.hint");
    decoder_ = quirc_new();
    if (!decoder_ || quirc_resize(decoder_, Width, Height) < 0)
        return fail(i18n::text("qr.err.memory"), -1);
    // Camera DMA needs physically contiguous, uncached memory, aligned to 1 MiB.
    memory_ = sceKernelAllocMemBlock("VitaPlaneCamera", SCE_KERNEL_MEMBLOCK_TYPE_USER_MAIN_PHYCONT_NC_RW,
                                     1024 * 1024, nullptr);
    if (memory_ < 0) return fail(i18n::text("cam.memory"), memory_);
    void* buffer = nullptr;
    int rc = sceKernelGetMemBlockBase(memory_, &buffer);
    if (rc < 0) return fail(i18n::text("cam.buffer"), rc);
    camera_buffer_ = static_cast<unsigned char*>(buffer);
    SceCameraInfo info{};
    info.size = sizeof(info);
    info.priority = SCE_CAMERA_PRIORITY_SHARE;
    info.format = SCE_CAMERA_FORMAT_YUV420_PLANE;
    info.resolution = SCE_CAMERA_RESOLUTION_640_480;
    info.framerate = SCE_CAMERA_FRAMERATE_15_FPS;
    info.range = 1;
    info.sizeIBase = Width * Height;
    info.sizeUBase = info.sizeVBase = Width * Height / 4;
    info.pIBase = buffer;
    info.pUBase = camera_buffer_ + info.sizeIBase;
    info.pVBase = camera_buffer_ + info.sizeIBase + info.sizeUBase;
    rc = sceCameraOpen(Device, &info);
    if (rc < 0) return fail(i18n::text("qr.err.open_rear"), rc);
    camera_open_ = true;
    rc = sceCameraStart(Device);
    if (rc < 0) return fail(i18n::text("cam.start_failed"), rc);
    camera_started_ = true;
    vita2d_wait_rendering_done();
    texture_ = vita2d_create_empty_texture(PreviewWidth, PreviewHeight);
    if (!texture_) return fail(i18n::text("qr.err.preview_memory"), -1);
    std::memset(vita2d_texture_get_datap(texture_), 0, vita2d_texture_get_stride(texture_) * PreviewHeight);
    preview_pixels_.resize(PreviewWidth * PreviewHeight);
    pthread_attr_t attr;
    pthread_attr_init(&attr);
    pthread_attr_setstacksize(&attr, 512 * 1024);
    rc = pthread_create(&thread_, &attr, entry, this);
    pthread_attr_destroy(&attr);
    if (rc) return fail(i18n::text("qr.err.start"), rc);
    thread_started_ = started_ = true;
    return true;
}

void QrScanner::stop() {
    performance::Scope measurement(performance::Metric::QrStop);
    stopping_ = true;
    if (thread_started_) { pthread_join(thread_, nullptr); thread_started_ = false; }
    if (camera_started_) { sceCameraStop(Device); camera_started_ = false; }
    if (camera_open_) { sceCameraClose(Device); camera_open_ = false; }
    if (memory_ >= 0) { sceKernelFreeMemBlock(memory_); memory_ = -1; }
    camera_buffer_ = nullptr;
    if (decoder_) { quirc_destroy(decoder_); decoder_ = nullptr; }
    if (texture_) { vita2d_wait_rendering_done(); vita2d_free_texture(texture_); texture_ = nullptr; }
    std::vector<uint32_t>().swap(preview_pixels_);
    preview_ready_ = false; started_ = false;
}

void* QrScanner::entry(void* self) { static_cast<QrScanner*>(self)->run(); return nullptr; }
void QrScanner::run() {
    cpu_scheduler::Thread role(cpu_scheduler::Role::Video);
    cpu_scheduler::Media workload(cpu_scheduler::Role::Video);
    uint64_t last_frame = UINT64_MAX;
    // Keep both identification attempts on one captured frame even if camera
    // DMA updates its buffer while the decoder is working.
    std::vector<uint8_t> frame(Width * Height);
    std::vector<uint32_t> sums((Width + 1) * (Height + 1));
    auto next_decode = std::chrono::steady_clock::time_point::min();
    unsigned decode_frame = 0;
    while (!stopping_) {
        SceCameraRead read{};
        read.size = sizeof(read); read.mode = 1; // Nonblocking latest-frame read.
        int rc = sceCameraRead(Device, &read);
        if (rc >= 0 && read.frame != last_frame) {
            last_frame = read.frame;
            std::memcpy(frame.data(), camera_buffer_, frame.size());
            pthread_mutex_lock(&mutex_);
            for (int y = 0; y < PreviewHeight; ++y)
                for (int x = 0; x < PreviewWidth; ++x) {
                    uint32_t v = frame[(y * 2) * Width + x * 2];
                    preview_pixels_[y * PreviewWidth + x] = 0xff000000u | v | (v << 8) | (v << 16);
                }
            preview_ready_ = true;
            pthread_mutex_unlock(&mutex_);
            const auto now = std::chrono::steady_clock::now();
            if (now < next_decode) { sceKernelDelayThread(10000); continue; }
            next_decode = now + std::chrono::milliseconds(200);
            // Alternate raw and locally thresholded frames: screen glare and
            // shadows can defeat quirc's single global threshold.
            if (decode_frame++ % 2) {
                for (int y = 0; y < Height; ++y) {
                    uint32_t row = 0;
                    for (int x = 0; x < Width; ++x) {
                        row += frame[y * Width + x];
                        sums[(y + 1) * (Width + 1) + x + 1] = sums[y * (Width + 1) + x + 1] + row;
                    }
                }
                for (int y = 0; y < Height; ++y) {
                    const int top = std::max(0, y - 24), bottom = std::min(Height, y + 25);
                    for (int x = 0; x < Width; ++x) {
                        const int left = std::max(0, x - 24), right = std::min(Width, x + 25);
                        const uint32_t sum = sums[bottom * (Width + 1) + right] - sums[top * (Width + 1) + right]
                            - sums[bottom * (Width + 1) + left] + sums[top * (Width + 1) + left];
                        const int mean = sum / ((bottom - top) * (right - left));
                        frame[y * Width + x] = frame[y * Width + x] + 7 < mean ? 0 : 255;
                    }
                }
            }
            i18n::Text scan_status = i18n::text("qr.hint");
            // Retry straight-side finder fitting for Telegram's rounded rings.
            for (int attempt = 0; attempt < 2 && !stopping_; ++attempt) {
                uint8_t* gray = quirc_begin(decoder_, nullptr, nullptr);
                std::memcpy(gray, frame.data(), frame.size());
                quirc_set_rounded_finders(decoder_, attempt);
                quirc_end(decoder_);
                for (int i = 0; i < quirc_count(decoder_); ++i) {
                    quirc_code code{}; quirc_data data{};
                    quirc_extract(decoder_, i, &code);
                    auto error = quirc_decode(&code, &data);
                    if (error != QUIRC_SUCCESS) { quirc_flip(&code); error = quirc_decode(&code, &data); }
                    if (error != QUIRC_SUCCESS || data.payload_len == 0) continue;
                    std::string payload(reinterpret_cast<char*>(data.payload), data.payload_len);
                    ProxyLink proxy; i18n::Text message;
                    auto parsed = parse_proxy_link(payload, proxy, message);
                    pthread_mutex_lock(&mutex_);
                    if (parsed == ProxyLinkResult::Valid) link_ = payload;
                    else scan_status = parsed == ProxyLinkResult::Invalid ? message : i18n::text("qr.no_link");
                    pthread_mutex_unlock(&mutex_);
                    if (parsed == ProxyLinkResult::Valid) return;
                }
            }
            pthread_mutex_lock(&mutex_); status_ = scan_status; pthread_mutex_unlock(&mutex_);
        } else if (rc < 0 && rc != (int)SCE_CAMERA_ERROR_ALREADY_READ && rc != (int)SCE_CAMERA_ERROR_NOT_ACTIVE &&
                   rc != (int)SCE_CAMERA_ERROR_TIMEOUT) {
            char message[120];
            std::snprintf(message, sizeof(message), "0x%08X", (unsigned)rc);
            i18n::Text text = i18n::text("qr.err.camera_read", {message});
            pthread_mutex_lock(&mutex_); status_ = text; pthread_mutex_unlock(&mutex_);
        }
        // Poll fresh preview frames independently of the five-Hz decode budget.
        sceKernelDelayThread(10000);
    }
}

void QrScanner::upload() {
    if (!texture_) return;
    vita2d_wait_rendering_done();
    pthread_mutex_lock(&mutex_);
    if (preview_ready_) {
        auto* dst = static_cast<unsigned char*>(vita2d_texture_get_datap(texture_));
        unsigned stride = vita2d_texture_get_stride(texture_);
        for (int y = 0; y < PreviewHeight; ++y)
            std::memcpy(dst + y * stride, preview_pixels_.data() + y * PreviewWidth, PreviewWidth * 4);
        preview_ready_ = false;
    }
    pthread_mutex_unlock(&mutex_);
}
std::string QrScanner::takeLink() {
    pthread_mutex_lock(&mutex_); std::string result; result.swap(link_); pthread_mutex_unlock(&mutex_);
    return result;
}
i18n::Text QrScanner::status() {
    pthread_mutex_lock(&mutex_); i18n::Text result = status_;
    result.refresh(); pthread_mutex_unlock(&mutex_);
    return result;
}
