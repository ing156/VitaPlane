#include "vpk_installer.h"
#include "vpk_package.h"
#include "update_core.h"
#include "package_promote.h"
#include "updater.h"
#include "i18n.h"
#include <vita2d.h>
#include <psp2/kernel/threadmgr.h>
#include <psp2/kernel/processmgr.h>
#include <pthread.h>
#include <atomic>
#include <cstdio>
#include <sys/stat.h>
#include <vector>

namespace {
constexpr const char* staging = "ux0:data/vg-vpk-install";
std::string short_text(const std::string& text, size_t limit) {
    if (text.size() <= limit) return text;
    size_t end = limit;
    while (end && (static_cast<unsigned char>(text[end]) & 0xc0) == 0x80) --end;
    return text.substr(0, end) + "…";
}
bool read(const std::string& path, std::vector<uint8_t>& bytes) {
    FILE* f = std::fopen(path.c_str(), "rb");
    if (!f) return false;
    std::fseek(f, 0, SEEK_END); long size = std::ftell(f); std::rewind(f);
    bool ok = size > 0 && size <= 1024 * 1024;
    if (ok) { bytes.resize(size); ok = std::fread(bytes.data(), 1, bytes.size(), f) == bytes.size(); }
    std::fclose(f); return ok;
}
struct Job {
    std::string path, error;
    vpk::Metadata metadata;
    bool prepare_modules = false, promote = false, prepare_helper = false, ok = false;
    std::atomic<bool> done{false};
    std::atomic<unsigned> percent{0};
};
void* worker(void* arg) {
    Job& job = *static_cast<Job*>(arg);
    if (job.prepare_modules) {
        job.ok = prepare_package_promoter(job.error);
    } else if (job.prepare_helper) {
        job.ok = Updater::prepareLocalInstaller(job.error);
    } else if (job.promote) {
        job.ok = promote_package(staging, job.error);
    } else {
        job.error = i18n::tr("doc.save_failed");
        job.ok = update::remove_tree(staging) && update::extract_zip(job.path, staging,
            [&](uint64_t done, uint64_t total) { job.percent = total ? unsigned(done * 100 / total) : 0; return true; }, job.error);
        std::vector<uint8_t> sfo, head;
        if (job.ok && (!read(std::string(staging) + "/sce_sys/param.sfo", sfo) || !vpk::parse_sfo(sfo, job.metadata))) {
            job.ok = false; job.error = i18n::tr("vpk.invalid");
        }
        if (job.ok && job.metadata.title_id == "VGUPDATE1") {
            job.ok = false; job.error = i18n::tr("vpk.self");
        }
        struct stat info{};
        if (job.ok && (stat((std::string(staging) + "/eboot.bin").c_str(), &info) != 0 || !S_ISREG(info.st_mode) || info.st_size < 0x88)) {
            job.ok = false; job.error = i18n::tr("vpk.invalid");
        }
        // Always create the header ourselves; never trust a header attached to a chat package.
        if (job.ok && (!read("app0:sce_sys/package/head.bin", head) || !vpk::make_head(head, job.metadata))) {
            job.ok = false; job.error = i18n::tr("vpk.header_failed");
        }
        if (job.ok) {
            std::string directory = std::string(staging) + "/sce_sys/package";
            job.ok = update::make_directories(directory);
            FILE* out = job.ok ? std::fopen((directory + "/head.bin").c_str(), "wb") : nullptr;
            job.ok = out && std::fwrite(head.data(), 1, head.size(), out) == head.size();
            if (out && std::fclose(out) != 0) job.ok = false;
            if (!job.ok) job.error = i18n::tr("doc.save_failed");
        }
    }
    job.done.store(true, std::memory_order_release);
    return nullptr;
}
bool run(Job& job, const std::function<void(const std::string&, int)>& progress) {
    const bool indeterminate = job.prepare_modules || job.promote || job.prepare_helper;
    const std::string text = i18n::tr(job.promote || job.prepare_helper ? "vpk.installing" : "vpk.extracting");
    auto show = [&](int percent) {
        sceKernelPowerTick(SCE_KERNEL_POWER_TICK_DEFAULT);
        progress(text, percent);
        vita2d_wait_rendering_done();
        sceGxmDisplayQueueFinish();
    };
    struct PowerLock {
        bool locked = sceKernelPowerLock(SCE_KERNEL_POWER_TICK_DEFAULT) >= 0;
        ~PowerLock() { if (locked) sceKernelPowerUnlock(SCE_KERNEL_POWER_TICK_DEFAULT); }
    } power_lock;
    job.done = false; job.percent = 0;
    show(indeterminate ? -1 : 0);
    pthread_t thread;
    if (pthread_create(&thread, nullptr, worker, &job) != 0) {
        job.error = i18n::tr("vpk.thread_failed"); return false;
    }
    while (!job.done.load(std::memory_order_acquire)) {
        show(indeterminate ? -1 : int(job.percent.load()));
    }
    pthread_join(thread, nullptr);
    if (job.ok && !indeterminate) show(100);
    return job.ok;
}
}
bool install_downloaded_vpk(const std::string& path,
                            const std::function<void(const std::string&, int)>& progress,
                            const std::function<bool(const std::string&, bool)>& dialog) {
    Job job; job.prepare_modules = true;
    if (!run(job, progress)) {
        dialog(i18n::tr("vpk.failed", {short_text(job.error, 240)}), false);
        return false;
    }
    job.prepare_modules = false;
    if (!dialog(i18n::tr("vpk.question", {short_text(path.substr(path.find_last_of('/') + 1), 120)}), true)) return false;
    job.path = path;
    bool ok = run(job, progress);
    if (ok && !dialog(i18n::tr("vpk.warning", {short_text(job.metadata.title, 80), job.metadata.title_id}), true)) {
        update::remove_tree(staging); return false;
    }
    if (ok && job.metadata.title_id == "VITAGRM01") {
        // Launch must run on the main thread. The helper uses the same handoff as signed updates,
        // while the local package is authorized only by these explicit confirmations.
        job.prepare_helper = true;
        if (run(job, progress)) {
            // The helper's system font does not contain CJK glyphs.
            const std::string language = i18n::current();
            bool cjk = language == "ja" || language == "ko" || language == "zh-Hans" || language == "zh-Hant";
            Updater::HandoffText text{cjk ? "Installing update..." : i18n::tr("update.applying"),
                cjk ? "Please wait. VitaPlane will restart." : i18n::tr("update.applying_hint")};
            if (Updater::handoffConfirmedPackage(staging, text, job.error)) return true;
        }
        ok = false;
    } else if (ok) { job.promote = true; ok = run(job, progress); }
    update::remove_tree(staging);
    std::string result = ok ? i18n::tr("vpk.success") : i18n::tr("vpk.failed", {short_text(job.error, 240)});
    dialog(result, false);
    return false;
}
