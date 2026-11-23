// Real installer/module lifecycle with Vita calls mocked; never installs a host package.
#include "../src/package_promote.cpp"
#include "../src/vpk_installer.cpp"
#include <cassert>
#include <chrono>
#include <cstring>
#include <filesystem>
#include <fstream>
#include <thread>

namespace fs = std::filesystem;
static std::thread::id ui;
static std::string scenario;
static std::atomic<bool> active{false}, promotion_started{false}, release_promotion{false};
static int busy_frames = 0, extraction_frames = 0;
static bool paf_loaded = false, promoter_loaded = false;
static int paf_loads = 0, promoter_loads = 0, messages = 0, extractions = 0, promotions = 0;
static int frames = 0, locks = 0, thread_calls = 0;
static bool fail_thread = false;
static std::string shown;

extern "C" {
void* sceClibMemset(void* dst, int ch, SceSize len) { return std::memset(dst, ch, len); }
int sceSysmoduleIsLoadedInternal(SceSysmoduleInternalModuleId id) {
    return (id == SCE_SYSMODULE_INTERNAL_PAF ? paf_loaded : promoter_loaded) ? int(SCE_SYSMODULE_LOADED) : -1;
}
int sceSysmoduleLoadModuleInternalWithArg(SceSysmoduleInternalModuleId id, SceSize, void*, const SceSysmoduleOpt* opt) {
    assert(std::this_thread::get_id() != ui && !active && id == SCE_SYSMODULE_INTERNAL_PAF);
    ++paf_loads;
    std::this_thread::sleep_for(std::chrono::milliseconds(5));
    if (scenario == "paf-load-error") return -5;
    *opt->result = scenario == "paf-start-error" ? -6 : 0;
    paf_loaded = *opt->result == 0;
    return 0;
}
int sceSysmoduleLoadModuleInternal(SceSysmoduleInternalModuleId id) {
    assert(std::this_thread::get_id() != ui && !active && paf_loaded && id == SCE_SYSMODULE_INTERNAL_PROMOTER_UTIL);
    ++promoter_loads;
    if (scenario == "promoter-error") return -7;
    if (promoter_loaded) return -8;
    promoter_loaded = true; return 0;
}
int scePromoterUtilityInit() { assert(paf_loaded && promoter_loaded && !active); return 0; }
int scePromoterUtilityExit() { assert(!active); return 0; }
int scePromoterUtilityDeletePkg(const char*) { return 0; }
int scePromoterUtilityPromotePkg(const char*, int) {
    assert(std::this_thread::get_id() != ui && !active); ++promotions;
    promotion_started = true;
    while (!release_promotion) std::this_thread::yield();
    return scenario == "promote-error" ? -9 : 0;
}
int scePromoterUtilityPromotePkgWithRif(const char* path, int sync) { return scePromoterUtilityPromotePkg(path, sync); }
int scePromoterUtilityGetState(int* state) { assert(!active); *state = 0; return 0; }
int scePromoterUtilityGetResult(int* result) { *result = 0; return 0; }
int sceKernelDelayThread(SceUInt) { return 0; }
int sceKernelPowerTick(SceKernelPowerTickType) { return 0; }
int sceKernelPowerLock(SceKernelPowerTickType) { ++locks; return 0; }
int sceKernelPowerUnlock(SceKernelPowerTickType) { --locks; return 0; }
int sceGxmDisplayQueueFinish() { return 0; }
void vita2d_start_drawing() { assert(std::this_thread::get_id() == ui); ++frames; }
void vita2d_clear_screen() {}
void vita2d_end_drawing() {}
void vita2d_wait_rendering_done() {}
void vita2d_swap_buffers() { std::this_thread::sleep_for(std::chrono::microseconds(50)); }
int __real_pthread_create(pthread_t*, const pthread_attr_t*, void* (*)(void*), void*);
int __wrap_pthread_create(pthread_t* t, const pthread_attr_t* a, void* (*entry)(void*), void* arg) {
    if (++thread_calls == 2 && scenario == "progress-thread-error") return EAGAIN;
    if (fail_thread) { fail_thread = false; return EAGAIN; }
    return __real_pthread_create(t, a, entry, arg);
}
}
namespace i18n {
std::string tr(const char* key) { return key; }
std::string tr(const char* key, std::initializer_list<std::string>) { return key; }
std::string current() { return "en"; }
}
namespace update {
bool remove_tree(const std::string& path) { fs::remove_all(path); return true; }
bool make_directories(const std::string& path) { fs::create_directories(path); return true; }
bool extract_zip(const std::string&, const std::string& dest, const std::function<bool(uint64_t,uint64_t)>&, std::string&) {
    assert(std::this_thread::get_id() != ui); ++extractions;
    fs::create_directories(dest + "/sce_sys");
    std::ofstream(dest + "/sce_sys/param.sfo") << "fixture";
    std::ofstream(dest + "/eboot.bin") << std::string(256, 'E');
    return true;
}
}
namespace vpk {
bool parse_sfo(const std::vector<uint8_t>&, Metadata& metadata) {
    metadata.title_id = scenario == "helper" ? "VITAGRM01" : "TEST00001";
    metadata.title = "Test"; return scenario != "invalid";
}
bool make_head(std::vector<uint8_t>&, const Metadata&) { return true; }
}
bool Updater::prepareLocalInstaller(std::string&) {
    assert(std::this_thread::get_id() != ui && !active);
    promotion_started = true;
    while (!release_promotion) std::this_thread::yield();
    std::string detail; return prepare_package_promoter(detail);
}
bool Updater::handoffConfirmedPackage(const std::string&, const HandoffText&, std::string&) { return true; }

int main(int argc, char** argv) {
    assert(argc == 2); scenario = argv[1]; ui = std::this_thread::get_id();
    paf_loaded = scenario == "loaded" || scenario == "promoter-loaded";
    promoter_loaded = scenario == "promoter-loaded";
    fail_thread = scenario == "thread-error";
    fs::create_directories("app0:sce_sys/package");
    std::ofstream("app0:sce_sys/package/head.bin") << "fixture";
    auto progress = [&](const std::string& text, int percent) {
        assert(std::this_thread::get_id() == ui && !active && locks == 1);
        assert(percent >= -1 && percent <= 100);
        if (text == "vpk.extracting") { if (percent >= 0) ++extraction_frames; }
        else { assert(text == "vpk.installing" && percent == -1); }
        if (promotion_started && !release_promotion) {
            assert(text == "vpk.installing" && percent == -1);
            ++busy_frames; release_promotion = true;
        }
        vita2d_start_drawing(); vita2d_swap_buffers();
    };
    auto dialog = [&](const std::string& text, bool question) {
        assert(std::this_thread::get_id() == ui && locks == 0);
        ++messages;
        shown = text;
        assert(question == (text == "vpk.question" || text == "vpk.warning"));
        return scenario != "cancel" && !(scenario == "decline-install" && messages == 2);
    };
    bool handoff = install_downloaded_vpk("fixture.vpk", progress, dialog);
    assert(!active && locks == 0 && handoff == (scenario == "helper"));
    if (scenario.find("paf-") == 0 || scenario == "promoter-error" || scenario == "thread-error") {
        assert(extractions == 0 && promotions == 0 && messages == 1 && shown == "vpk.failed");
    } else {
        assert(paf_loads == (scenario == "loaded" || scenario == "promoter-loaded" ? 0 : 1));
        assert(promoter_loads == 1);
        if (scenario == "cancel") assert(extractions == 0 && promotions == 0 && messages == 1);
        else if (scenario == "progress-thread-error")
            assert(extractions == 0 && promotions == 0 && shown == "vpk.failed");
        else if (scenario == "decline-install")
            assert(extractions == 1 && promotions == 0 && messages == 2 && !fs::exists(staging));
        else {
            assert(extractions == 1 && extraction_frames > 0);
            assert(promotions == (scenario == "invalid" || scenario == "helper" ? 0 : scenario == "promote-error" ? 2 : 1));
            if (promotions || handoff) assert(busy_frames > 0);
            if (!handoff) assert(shown == (scenario == "invalid" || scenario == "promote-error" ? "vpk.failed" : "vpk.success"));
        }
        // Cached preparation must not touch modules while a dialog is live.
        active = true; std::string detail; assert(prepare_package_promoter(detail)); active = false;
        assert(promoter_loads == 1);
        if (scenario == "fresh") assert(frames > 0);
    }
    if (scenario == "repeat") {
        promotion_started = false; release_promotion = false;
        assert(!install_downloaded_vpk("second.vpk", progress, dialog));
        assert(promotions == 2 && busy_frames == 2 && messages == 6);
        assert(promoter_loads == 1 && paf_loads == 1 && !active && locks == 0);
    }
    std::puts("VPK installer lifecycle passed");
}
