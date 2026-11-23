#pragma once
#include "diag_log.h"
#include <array>
#include <chrono>
#include <cstdint>
#include <mutex>
#include <sys/stat.h>

// Producers enqueue fixed names and numbers only; the Storage worker owns file I/O.
namespace proxy_diagnostics {
enum class Stage : unsigned {
    Apply, Check, Queued, Sent, Parameters, Result, Timeout, Cancel, Connection, Switch,
    Request, DnsSystem, DnsCache, DnsHttps, Tcp, Ca, Tls, HttpIo, HttpStatus,
    Bootstrap, Session, Carrier, Listen, Downlink, Retry, Failed, Startup, DnsBegin, DnsBootstrap, Count
};
enum class Endpoint : unsigned { Control, Bridge, Session, Up, Down, Delete, Dns, Count };
struct Event {
    int64_t at=0;uint64_t id=0;
    Stage stage=Stage::Failed;Endpoint endpoint=Endpoint::Control;
    int64_t code=0,detail=0,aux=0;
};
struct Ring {
    std::mutex mutex;
    std::array<Event,128> events{};
    size_t count=0;uint64_t dropped=0;
    int64_t next_flush=0;
};
inline Ring& ring() { static Ring value;return value; }
inline int64_t now() {
    return std::chrono::duration_cast<std::chrono::microseconds>(std::chrono::steady_clock::now().time_since_epoch()).count();
}
inline uint64_t next_id() { static std::atomic<uint64_t> value{0};return ++value; }
inline void record(Stage stage,uint64_t id,int64_t code=0,int64_t detail=0,
                   Endpoint endpoint=Endpoint::Control,int64_t aux=0) {
    if (!diag_log::enabled(diag_log::Kind::Proxy) || unsigned(stage)>=unsigned(Stage::Count) ||
        unsigned(endpoint)>=unsigned(Endpoint::Count)) return;
    auto& r=ring();std::lock_guard<std::mutex> lock(r.mutex);
    if (r.count==r.events.size()) { ++r.dropped;return; }
    r.events[r.count++]={now(),id,stage,endpoint,code,detail,aux};
}
inline void flush(bool force=false) {
    static constexpr const char* stages[]={
        "apply","check","queued","sent","parameters","result","timeout","cancel","connection","switch",
        "request","dns_system","dns_cache","dns_https","tcp","ca","tls","http_io","http_status",
        "bootstrap","session","carrier","listen","downlink","retry","failed","startup","dns_begin","dns_bootstrap"
    };
    static constexpr const char* endpoints[]={"control","bridge","session","up","down","delete","dns"};
    static_assert(sizeof(stages)/sizeof(*stages)==unsigned(Stage::Count));
    static_assert(sizeof(endpoints)/sizeof(*endpoints)==unsigned(Endpoint::Count));
    std::array<Event,128> events;size_t count;uint64_t dropped;
    auto& r=ring();
    {
        std::lock_guard<std::mutex> lock(r.mutex);
        if (!diag_log::enabled(diag_log::Kind::Proxy)) { r.count=0;r.dropped=0;return; }
        if (!force && now()<r.next_flush) return;
        r.next_flush=now()+1000000;
        count=r.count;dropped=r.dropped;events=r.events;r.count=0;r.dropped=0;
    }
    if (!count && !dropped) return;
    const char* path="ux0:data/vitagram/proxy.log";
    struct stat info{};
    bool ok=true;
    // Reserve the maximum batch size before appending; retain one previous file.
    if (!stat(path,&info) && info.st_size+static_cast<int64_t>((count+1)*256)>256*1024)
        ok=std::rename(path,"ux0:data/vitagram/proxy.previous.log")==0;
    FILE* file=ok?diag_log::open(diag_log::Kind::Proxy,path,"ab"):nullptr;
    if (file) {
#ifdef VITAGRAM_VERSION
        constexpr const char* version=VITAGRAM_VERSION;
#else
        constexpr const char* version="host-test";
#endif
        ok=std::fprintf(file,"proxy version=%s omitted=%llu\n",version,(unsigned long long)dropped)>0;
        for (size_t i=0;i<count && ok;++i) {
            const auto& e=events[i];
            ok=std::fprintf(file,"at_us=%lld id=%llu stage=%s endpoint=%s code=%lld detail=%lld aux=%lld\n",
                (long long)e.at,(unsigned long long)e.id,stages[unsigned(e.stage)],endpoints[unsigned(e.endpoint)],
                (long long)e.code,(long long)e.detail,(long long)e.aux)>0;
        }
        if (std::fclose(file)) ok=false;
    } else ok=false;
    if (!ok) { std::lock_guard<std::mutex> lock(r.mutex);r.dropped+=count+dropped; }
}
}
