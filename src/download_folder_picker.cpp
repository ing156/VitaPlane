#include "download_folder_picker.h"
#include "i18n.h"
#include "file_download.h"
#include <psp2/io/dirent.h>
#include <algorithm>

DownloadFolderPicker::~DownloadFolderPicker() {
    {
        std::lock_guard<std::mutex> lock(mutex_);
        stopping_ = true; ++generation_;
    }
    condition_.notify_one();
    if (started_) pthread_join(thread_, nullptr);
}

void DownloadFolderPicker::submit(Request request) {
    loading = true; entries.clear(); selected = 0; error.clear();
    std::lock_guard<std::mutex> lock(mutex_);
    request.generation = ++generation_;
    request_ = std::move(request); pending_ = true; ready_ = false;
    if (!started_) {
        pthread_attr_t attr;
        pthread_attr_init(&attr);
        pthread_attr_setstacksize(&attr, 256 * 1024);
        const int rc = pthread_create(&thread_, &attr, worker, this);
        pthread_attr_destroy(&attr);
        if (rc != 0) { pending_ = loading = false; error = i18n::text("picker.open_failed"); return; }
        started_ = true;
    }
    condition_.notify_one();
}

void* DownloadFolderPicker::worker(void* self) {
    auto& picker = *static_cast<DownloadFolderPicker*>(self);
    for (;;) {
        Request request;
        {
            std::unique_lock<std::mutex> lock(picker.mutex_);
            picker.condition_.wait(lock, [&] { return picker.stopping_ || picker.pending_; });
            if (picker.stopping_) return nullptr;
            request = std::move(picker.request_); picker.pending_ = false;
        }
        auto result = picker.scan(request);
        std::lock_guard<std::mutex> lock(picker.mutex_);
        if (request.generation == picker.generation_) { picker.result_ = std::move(result); picker.ready_ = true; }
    }
}

void DownloadFolderPicker::poll() {
    std::lock_guard<std::mutex> lock(mutex_);
    if (!active || !ready_) return;
    path = std::move(result_.path); entries = std::move(result_.entries); error = std::move(result_.error);
    error.refresh(); ready_ = loading = false;
}

void DownloadFolderPicker::close() {
    active = loading = false; entries.clear();
    std::lock_guard<std::mutex> lock(mutex_);
    ++generation_; pending_ = ready_ = false; result_ = Listing{};
}

void DownloadFolderPicker::open(const std::string& preferred, const std::string& fallback) {
    active = true; path = normalized_download_dir(preferred);
    submit({path, fallback, {}, true, 0});
}

void DownloadFolderPicker::reload() { submit({path, {}, {}, false, 0}); }

DownloadFolderPicker::Listing DownloadFolderPicker::scan(const Request& request) {
    Listing result; result.path = request.path;
    auto current = [&] { return request.generation == generation_; };
    if (request.resolve) {
        if (!current()) return result;
        if (!exists(result.path)) result.path = normalized_download_dir(request.fallback);
        if (!current()) return result;
        if (!exists(result.path)) result.path = "ux0:downloads";
        if (!current()) return result;
        if (!exists(result.path)) result.path = "ux0:";
        if (!current()) return result;
        if (!exists(result.path)) result.path.clear();
    }
    if (!request.name.empty() && current()) {
        const auto target = result.path + (result.path.back() == ':' ? "" : "/") + request.name;
        if (sceIoMkdir(target.c_str(), 0777) < 0) result.error = i18n::text("picker.create_failed");
        else result.path = target;
    }
    if (result.path.empty()) {
        for (const char* root : {"ux0:", "uma0:", "imc0:", "ur0:", "grw0:"}) {
            if (!current()) return result;
            if (exists(root)) result.entries.push_back({root, true});
        }
        if (result.entries.empty()) result.error = i18n::text("picker.no_drives");
        return result;
    }
    if (!current()) return result;
    const auto directory = result.path.back() == ':' ? result.path + "/" : result.path;
    const SceUID dir = sceIoDopen(directory.c_str());
    if (dir < 0) { result.error = i18n::text("picker.open_failed"); return result; }
    SceIoDirent entry{};
    int rc = 0;
    // shortcut: first 5000 entries, add paged scanning if larger folders are needed.
    while (current() && result.entries.size() < 5000 && (rc = sceIoDread(dir, &entry)) > 0) {
        std::string name(entry.d_name);
        if (name != "." && name != "..") result.entries.push_back({name, bool(SCE_S_ISDIR(entry.d_stat.st_mode))});
        entry = {};
    }
    sceIoDclose(dir);
    if (rc < 0) result.error = i18n::text("picker.open_failed");
    std::sort(result.entries.begin(), result.entries.end(), [](const Entry& a, const Entry& b) {
        return a.dir != b.dir ? a.dir > b.dir : a.name < b.name;
    });
    return result;
}

void DownloadFolderPicker::enter() {
    if (loading || selected < 0 || selected >= (int)entries.size() || !entries[selected].dir) return;
    path = path.empty() ? entries[selected].name : path + (path.back() == ':' ? "" : "/") + entries[selected].name;
    reload();
}

void DownloadFolderPicker::up() {
    if (path.empty()) { close(); return; }
    const size_t slash = path.find_last_of('/');
    path = slash == std::string::npos ? std::string() : path.substr(0, slash);
    reload();
}

bool DownloadFolderPicker::create(const std::string& name) {
    if (loading) return false;
    if (path.empty() || name.empty() || name == "." || name == ".." || name.size() > 180 ||
        name.find_first_of("/\\:*?\"<>|") != std::string::npos) {
        error = i18n::text("picker.invalid_name"); return false;
    }
    for (unsigned char c : name) if (c < 32) { error = i18n::text("picker.invalid_name"); return false; }
    submit({path, {}, name, false, 0});
    return true;
}

bool DownloadFolderPicker::exists(const std::string& directory) {
    if (directory.empty()) return false;
    const auto target = directory.back() == ':' ? directory + "/" : directory;
    const SceUID dir = sceIoDopen(target.c_str());
    if (dir < 0) return false;
    sceIoDclose(dir);
    return true;
}
