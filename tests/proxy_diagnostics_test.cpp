#include "proxy_diagnostics.h"
#include <cassert>
#include <filesystem>
#include <fstream>
#include <iterator>
#include <thread>
#include <vector>
#include <cerrno>
#include <netdb.h>

static std::thread::id ui;
static bool fail_open=false;
extern "C" FILE* __real_fopen(const char*,const char*);
extern "C" FILE* __wrap_fopen(const char* path,const char* mode) {
    assert(std::this_thread::get_id()!=ui);
    if (fail_open) { errno=EACCES;return nullptr; }
    return __real_fopen(path,mode);
}
extern "C" int __real_rename(const char*,const char*);
extern "C" int __wrap_rename(const char* a,const char* b) {
    assert(std::this_thread::get_id()!=ui);return __real_rename(a,b);
}
extern "C" int __real_stat(const char*,struct stat*);
extern "C" int __wrap_stat(const char* path,struct stat* info) {
    assert(std::this_thread::get_id()!=ui);return __real_stat(path,info);
}
static void flush_worker() { std::thread worker([] { proxy_diagnostics::flush(true); });worker.join(); }
static std::string contents() {
    std::ifstream in("ux0:data/vitagram/proxy.log");
    return {std::istreambuf_iterator<char>(in),std::istreambuf_iterator<char>()};
}
int main() {
    using namespace proxy_diagnostics;
    ui=std::this_thread::get_id();
    std::filesystem::create_directories("ux0:data/vitagram");
    const auto path=std::filesystem::path("ux0:data/vitagram/proxy.log");
    const auto previous=std::filesystem::path("ux0:data/vitagram/proxy.previous.log");
    assert(diag_log::enabled(diag_log::Kind::Proxy));
    assert(diag_log::mask().load()==diag_log::kStartupMask);
    record(Stage::Startup,0);flush_worker();assert(std::filesystem::exists(path));
    assert(contents().find("stage=startup")!=std::string::npos);
    record(Stage::DnsBegin,41,0,4,Endpoint::Dns,1);
    record(Stage::DnsSystem,41,EAI_AGAIN,ETIMEDOUT,Endpoint::Dns,4750000);
    record(Stage::DnsHttps,42,500,403,Endpoint::Dns,100000);
    record(Stage::DnsBootstrap,43,0,0,Endpoint::Dns,10);
    flush_worker();
    assert(contents().find("stage=dns_begin endpoint=dns code=0 detail=4 aux=1")!=std::string::npos);
    assert(contents().find("stage=dns_system endpoint=dns code="+std::to_string(EAI_AGAIN))!=std::string::npos);
    assert(contents().find("stage=dns_https endpoint=dns code=500 detail=403 aux=100000")!=std::string::npos);
    assert(contents().find("stage=dns_bootstrap endpoint=dns code=0 detail=0 aux=10")!=std::string::npos);
    std::filesystem::remove(path);
    diag_log::set_mask(0);
    record(Stage::Apply,1);flush_worker();assert(!std::filesystem::exists(path));
    diag_log::set_mask(1u<<unsigned(diag_log::Kind::Proxy));
    record(Stage::Apply,42,3,2);record(Stage::Tcp,42,ECONNREFUSED,0,Endpoint::Bridge);
    record(Stage::Count,42);record(Stage::Apply,42,0,0,Endpoint::Count);
    assert(!std::filesystem::exists(path));flush_worker();
    auto log=contents();
    assert(log.find("version=diagnostics-test")!=std::string::npos);
    assert(log.find("id=42 stage=apply endpoint=control code=3 detail=2")!=std::string::npos);
    assert(log.find("stage=tcp endpoint=bridge code="+std::to_string(ECONNREFUSED))!=std::string::npos);
    std::vector<std::thread> producers;
    for (int i=0;i<4;++i) producers.emplace_back([] { for (int j=0;j<128;++j) record(Stage::Sent,next_id()); });
    for (auto& t:producers) t.join();
    flush_worker();
    assert(contents().find("omitted=384")!=std::string::npos);
    for (int pass=0;pass<2;++pass) {
        { std::ofstream out(path);out<<std::string(256*1024-32,'x'); }
        record(Stage::Result,43);flush_worker();
        assert(std::filesystem::file_size(previous)==256*1024-32);
        assert(std::filesystem::file_size(path)<256*1024);
    }
    fail_open=true;record(Stage::Tls,44,1);flush_worker();fail_open=false;
    record(Stage::Result,44);flush_worker();assert(contents().find("omitted=1")!=std::string::npos);
    const auto size=std::filesystem::file_size(path);
    record(Stage::Apply,45);diag_log::set_mask(0);flush_worker();
    record(Stage::Check,46);flush_worker();assert(std::filesystem::file_size(path)==size);
    assert((diag_log::kAll & 63u)==63u); // Previously saved log bits keep their meaning.
    std::puts("Proxy diagnostics: automatic startup, explicit disable, typed events, concurrent bounded queue, worker-only I/O, rotation and write-loss reporting PASS");
}
