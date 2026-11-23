#pragma once
#include "diag_log.h"
#include "performance.h"
#include <cstdio>
#include <mutex>

namespace document_diagnostics {
enum class Stage { BytesFull, TdComplete, MissingPath, PathMismatch, Paused, RenameQueued,
                   RenameStarted, RenameOk, RenameError, ResultIgnored, RowRemoved };
constexpr const char* names[] = {"bytes_full", "td_complete", "missing_path", "path_mismatch", "paused",
    "rename_queued", "rename_started", "rename_ok", "rename_error", "result_ignored", "row_removed"};
struct Event { int64_t at; unsigned attempt; Stage stage; int error; };
struct Batch {
    std::array<Event, 64> events{};
    size_t count = 0;
    unsigned omitted = 0;
};
inline Batch pending;
inline std::mutex mutex;

inline void record(Stage stage, unsigned attempt, int error = 0, unsigned* seen = nullptr) {
    if (!performance::enabled || !diag_log::enabled(diag_log::Kind::Performance)) return;
    if (seen) {
        const unsigned bit = 1u << static_cast<unsigned>(stage);
        if (*seen & bit) return;
        *seen |= bit;
    }
    std::lock_guard<std::mutex> lock(mutex);
    if (pending.count == pending.events.size()) { ++pending.omitted; return; }
    pending.events[pending.count++] = {performance::now(), attempt, stage, error};
}

inline void write(FILE* file) {
    Batch batch;
    {
        std::lock_guard<std::mutex> lock(mutex);
        batch = pending;
        pending.count = 0;
        pending.omitted = 0;
    }
    for (size_t i = 0; i < batch.count; ++i) {
        const auto& e = batch.events[i];
        std::fprintf(file, "document at_us=%lld attempt=%u stage=%s errno=%d\n",
            static_cast<long long>(e.at), e.attempt, names[static_cast<unsigned>(e.stage)], e.error);
    }
    if (batch.omitted) std::fprintf(file, "document omitted=%u\n", batch.omitted);
}
}
