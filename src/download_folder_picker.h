#pragma once
#include "i18n.h"
#include <string>
#include <vector>
#include <atomic>
#include <mutex>
#include <condition_variable>
#include <pthread.h>

class DownloadFolderPicker {
public:
    ~DownloadFolderPicker();
    bool active = false, loading = false;
    std::string path; i18n::Text error;
    struct Entry { std::string name; bool dir; };
    std::vector<Entry> entries;
    static constexpr int rows = 8, row_height = 43, row_fill = 40, list_top = 137;
    static constexpr int list_left = 32, list_width = 896;
    static constexpr int footer_left = 20, footer_step = 230, footer_width = 220;
    int hit(int x, int y) const {
        const int local = y-list_top;
        if (x < list_left || x >= list_left+list_width || local < 0 || local >= rows*row_height || local%row_height >= row_fill) return -1;
        const int index = selected/rows*rows + local/row_height;
        return index < (int)entries.size() ? index : -1;
    }
    static int footerHit(int x, int y) {
        const int local = x-footer_left;
        return y >= 508 && y < 540 && local >= 0 && local < 4*footer_step && local%footer_step < footer_width ? local/footer_step : -1;
    }
    int selected = 0;

    void open(const std::string& preferred, const std::string& fallback = {});
    void reload();
    void poll();
    void close();
    void enter();
    void up();
    bool create(const std::string& name);
private:
    struct Request { std::string path, fallback, name; bool resolve = false; unsigned generation = 0; };
    struct Listing { std::string path; i18n::Text error; std::vector<Entry> entries; };
    pthread_t thread_{};
    bool started_ = false, stopping_ = false, pending_ = false, ready_ = false;
    std::mutex mutex_;
    std::condition_variable condition_;
    std::atomic<unsigned> generation_{0};
    Request request_;
    Listing result_;
    void submit(Request request);
    static void* worker(void* self);
    Listing scan(const Request& request);
    static bool exists(const std::string& directory);
};
