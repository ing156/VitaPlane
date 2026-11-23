#include "proxy_settings_save.h"
#include <sys/stat.h>
#include <unistd.h>
#include <cassert>
#include <thread>
#include <atomic>
#include <fstream>
#include <iostream>

static std::atomic<bool> watch_io{false};
static std::thread::id ui;
extern "C" FILE* __real_fopen(const char*,const char*);
extern "C" FILE* __wrap_fopen(const char* p,const char* m) {
    if (watch_io) assert(std::this_thread::get_id()!=ui);
    return __real_fopen(p,m);
}
extern "C" int __real_rename(const char*,const char*);
extern "C" int __wrap_rename(const char* a,const char* b) {
    if (watch_io) assert(std::this_thread::get_id()!=ui);
    return __real_rename(a,b);
}
static void write(const std::string& text) { std::ofstream f("ux0:data/vitagram/settings.json");f<<text;assert(f.good()); }
static std::string read() { std::ifstream f("ux0:data/vitagram/settings.json");return {std::istreambuf_iterator<char>(f),{}}; }
int main() {
    ui=std::this_thread::get_id();assert(!mkdir("ux0:data",0777));assert(!mkdir("ux0:data/vitagram",0777));
    write(R"({"proxy_enabled":true,"proxy_server":"legacy.example","proxy_port":443,"proxy_secret":"legacy-test-secret"})");
    AppSettings settings;settings.load();
    assert(settings.proxy_switch_seconds==10);
    settings.proxy_switch_seconds=60;settings.proxy_auto_switch=false;
    assert(settings.proxies.size()==1 && settings.active_proxy==0 && settings.proxies[0].secret=="legacy-test-secret");
    settings.proxy_type=1;settings.proxy_server="socks.example";settings.proxy_port=1080;
    settings.proxy_secret.clear();settings.proxy_username="user";settings.proxy_password="test-password";
    assert(settings.storeProxyDraft(-1) && settings.proxies.size()==2 && settings.active_proxy==1);
    assert(settings.storeProxyDraft(-1) && settings.proxies.size()==2); // Duplicate QR import.
    settings.proxy_password=std::string(256,'x');assert(!settings.storeProxyDraft(-1) && settings.proxies.size()==2);
    settings.proxy_password="test-password";
    settings.proxy_server=std::string(254,'a');assert(!settings.storeProxyDraft(-1));settings.proxy_server="socks.example";
    settings.proxy_secret=std::string(1024,'a');assert(!settings.storeProxyDraft(-1));settings.proxy_secret.clear();
    ProxySettingsSave job;AppSettings completed;bool ok=false;
    watch_io=true;assert(job.begin(settings));assert(!job.begin(settings));
    for (int i=0;i<1000 && !job.finish(completed,ok);++i) usleep(1000);
    watch_io=false;assert(!job.active() && ok);
    AppSettings loaded;loaded.load();
    assert(loaded.proxy_switch_seconds==60 && !loaded.proxy_auto_switch);
    assert(loaded.proxies.size()==2 && loaded.active_proxy==1 && loaded.proxy_type==1 && loaded.proxy_password=="test-password");
    loaded.eraseProxy(0);assert(loaded.active_proxy==0 && loaded.proxies[0].server=="socks.example");
    loaded.eraseProxy(0);assert(loaded.active_proxy==-1 && loaded.proxy_enabled && loaded.proxy_server.empty());
    assert(loaded.save());loaded.load();assert(loaded.proxies.empty() && loaded.proxy_enabled); // Last deletion cannot resurrect legacy fields or enable direct networking.
    const auto original=read();
    assert(!rename("ux0:data/vitagram/settings.json","saved.json"));assert(!mkdir("ux0:data/vitagram/settings.json",0777));
    assert(job.begin(settings));ok=true;
    for (int i=0;i<1000 && !job.finish(completed,ok);++i) usleep(1000);
    assert(!job.active() && !ok);
    assert(!rmdir("ux0:data/vitagram/settings.json"));assert(!rename("saved.json","ux0:data/vitagram/settings.json"));assert(read()==original);
    write(R"({"proxy_server":"partial.example","proxy_secret":""})");loaded.load();assert(loaded.proxies.size()==1 && !loaded.proxies[0].complete());
    assert(loaded.proxy_switch_seconds==10);
    write(R"({"proxy_switch_seconds":7})");loaded.load();assert(loaded.proxy_switch_seconds==10);
    write(R"({"proxies":[{"type":3,"server":"web.example/Path","port":80,"secret":"test-secret"}],"active_proxy":999})");
    loaded.load();assert(loaded.active_proxy==0 && loaded.proxyConfig().port==443 && loaded.proxy_server=="web.example/Path");
    vault::set_config_path("ux0:data/vitagram/vault.json");vault::load();
    assert(vault::enable(vault::Pin{0,4,2,6,7,3,1,5,4,0,6,2})==vault::Result::Ok);
    assert(settings.save() && vault::is_sealed("ux0:data/vitagram/settings.json"));
    assert(read().find("test-password")==std::string::npos && read().find("legacy-test-secret")==std::string::npos);
    loaded.load();assert(loaded.proxies.size()==2 && loaded.proxy_password=="test-password");
    std::cout<<"Proxy settings: legacy migration, duplicates, active/delete, failed atomic save, worker-only I/O and sealed secrets PASS\n";
}
