#include "web_proxy.h"
#include "web_proxy_http.h"
#include "proxy_diagnostics.h"
#include <filesystem>
#include <fstream>
#include <iterator>
#include <netdb.h>
#include <netinet/in.h>
#include <sys/socket.h>
#include <unistd.h>
#include <cassert>
#include <chrono>
#include <thread>
#include <iostream>
#include <cstdlib>
#include <csignal>
#include <cstring>
#include <atomic>
#include <pthread.h>
#include <cerrno>

static std::atomic<int> tls_threads{0};
static std::atomic<bool> reject_thread{false}, reject_stack{false}, reject_downlink{false};
static pthread_t main_thread;
extern "C" int __real_pthread_attr_setstacksize(pthread_attr_t*,size_t);
extern "C" int __wrap_pthread_attr_setstacksize(pthread_attr_t* attr,size_t size) {
    if (reject_stack.exchange(false)) return EINVAL;
    return __real_pthread_attr_setstacksize(attr,size);
}
extern "C" int __real_pthread_create(pthread_t*,const pthread_attr_t*,void*(*)(void*),void*);
extern "C" int __wrap_pthread_create(pthread_t* thread,const pthread_attr_t* attr,void*(*entry)(void*),void* data) {
    size_t size=0;
    assert(attr && pthread_attr_getstacksize(attr,&size)==0 && size==256*1024);
    if (reject_thread.exchange(false)) return EAGAIN;
    if (!pthread_equal(pthread_self(),main_thread) && reject_downlink.exchange(false)) return EAGAIN;
    ++tls_threads;
    return __real_pthread_create(thread,attr,entry,data);
}

// Redirect only the test relay's DNS/service. TLS still checks its real certificate and host.
extern "C" int __real_getaddrinfo(const char*,const char*,const addrinfo*,addrinfo**);
static std::atomic<int> doh_queries{0};
extern "C" int __wrap_getaddrinfo(const char* host,const char*,const addrinfo* hints,addrinfo** out) {
    if (std::string(host)=="8.8.8.8") { assert(hints->ai_flags & AI_NUMERICHOST);++doh_queries; }
    else if (std::string(host)!="127.0.0.1") { *out=nullptr;return EAI_AGAIN; }
    else assert(hints->ai_flags & AI_NUMERICHOST);
    return __real_getaddrinfo("127.0.0.1",std::getenv("WEB_PROXY_TEST_PORT"),hints,out);
}
static WebProxy::Snapshot wait(WebProxy& proxy) {
    for (int i=0;i<500;++i) {
        const auto state=proxy.snapshot();
        if (state.port || !state.error.empty()) return state;
        std::this_thread::sleep_for(std::chrono::milliseconds(10));
    }
    assert(false && "WEB worker timed out");return {};
}
static void echo(int port,int seed,size_t size=256*1024) {
    const int fd=socket(AF_INET,SOCK_STREAM,0);assert(fd>=0);
    timeval timeout{10,0};setsockopt(fd,SOL_SOCKET,SO_SNDTIMEO,&timeout,sizeof(timeout));
    setsockopt(fd,SOL_SOCKET,SO_RCVTIMEO,&timeout,sizeof(timeout));
    sockaddr_in local{};local.sin_family=AF_INET;local.sin_addr.s_addr=htonl(INADDR_LOOPBACK);local.sin_port=htons(port);
    assert(connect(fd,reinterpret_cast<sockaddr*>(&local),sizeof(local))==0);
    if (size>1024*1024) { int capacity=65536;setsockopt(fd,SOL_SOCKET,SO_RCVBUF,&capacity,sizeof(capacity)); }
    std::string data(size,'\0');
    for (size_t i=0;i<data.size();++i) data[i]=char((i*17+seed)%256);
    for (size_t at=0;at<data.size();) { int n=send(fd,data.data()+at,data.size()-at,0);assert(n>0);at+=n; }
    std::string received(data.size(),'\0');
    for (size_t at=0;at<received.size();) { int n=recv(fd,received.data()+at,received.size()-at,0);assert(n>0);at+=n; }
    assert(received==data);close(fd);
}
int main(int argc,char** argv) {
    std::signal(SIGPIPE,SIG_IGN);
    main_thread=pthread_self();
    assert(argc==2);
    std::filesystem::create_directories("ux0:data/vitagram");
    diag_log::set_mask(1u<<unsigned(diag_log::Kind::Proxy));
    auto flush_logs=[] { std::thread storage([] { proxy_diagnostics::flush(true); });storage.join(); };
    auto log_text=[] {
        std::ifstream in("ux0:data/vitagram/proxy.log");
        return std::string(std::istreambuf_iterator<char>(in),std::istreambuf_iterator<char>());
    };
    WebProxy proxy;ProxyConfig config;config.kind=ProxyKind::Web;
    config.server="proxy.example.com";config.secret="000102030405060708090a0b0c0d0e0f";
    {
        WebProxy unavailable;
        reject_stack=true;unavailable.configure(config);
        assert(unavailable.snapshot().port==0 && unavailable.snapshot().error=="proxy.web.failed");
        reject_thread=true;unavailable.configure(config);
        assert(unavailable.snapshot().port==0 && unavailable.snapshot().error=="proxy.web.failed");
    }
    if (std::string(argv[1])=="normal") {
        WebProxy unavailable;
        reject_downlink=true;unavailable.configure(config);
        const auto failed=wait(unavailable);
        assert(failed.port==0 && failed.error=="proxy.web.failed");
    }
    const int dns_before=doh_queries;
    proxy.configure(config);
    auto state=wait(proxy);
    assert(tls_threads>=1);
    if (std::string(argv[1])=="unsupported") {
        assert(state.port==0 && state.error=="proxy.web.unsupported");
        flush_logs();assert(log_text().find("stage=carrier endpoint=control code=3")!=std::string::npos);
        std::cout << "WEB unsupported carrier fails explicitly PASS\n";return 0;
    }
    if (std::string(argv[1])=="denied") {
        assert(state.port==0 && state.error=="proxy.web.failed");flush_logs();
        const auto log=log_text();
        assert(log.find("stage=session endpoint=control code=403")!=std::string::npos);
        assert(log.find("private-relay-debug-secret")==std::string::npos);
        std::cout<<"WEB diagnostics: session HTTP rejection and body privacy PASS\n";return 0;
    }
    assert(state.error.empty() && state.port>0);
    std::thread first(echo,state.port,3,256*1024),second(echo,state.port,91,256*1024);first.join();second.join();
    std::thread bulk_first(echo,state.port,7,6*1024*1024),bulk_second(echo,state.port,19,6*1024*1024);
    bulk_first.join();bulk_second.join();
    flush_logs();
    assert(doh_queries==dns_before+1); // Up/down requests reuse the bounded TTL cache.
    const auto began=std::chrono::steady_clock::now();proxy.cancel();
    assert(std::chrono::steady_clock::now()-began<std::chrono::milliseconds(20));
    assert(proxy.snapshot().port==0);
    config.server="wrong.example.com";proxy.configure(config);
    state=wait(proxy);assert(!state.port && state.error=="proxy.web.tls");
    config.server="proxy.example.com/AbC_1";proxy.configure(config);
    state=wait(proxy);assert(state.error.empty() && state.port>0);echo(state.port,17);
    config.server="bad";proxy.configure(config);
    state=wait(proxy);assert(!state.port && state.error=="proxy.web.invalid");
    config.server="proxy.example.com";proxy.configure(config);
    state=wait(proxy);assert(state.error.empty() && state.port>0);echo(state.port,29);
    proxy.cancel();
    assert(tls_threads>=4); // Persistent worker plus a downlink for each successful session.
    {
        web_proxy::Http http("app0:certs/cacert.pem");std::atomic<bool> cancelled{false};
        for (const char* host:{"noanswer.example.com","nxdomain.example.com","malformed.example.com"}) {
            const auto bad=http.request(host,"/","GET","","","",cancelled,1000);
            assert(bad.error=="proxy.check.dns" && bad.status==0);
        }
        cancelled=true;const int before=doh_queries;
        http.request("proxy.example.com","/","GET","","","",cancelled,1000);
        assert(doh_queries==before);
    }
    flush_logs();
    const auto log=log_text();
    for (const char* stage:{"dns_system","dns_https","dns_bootstrap","tcp","tls","bootstrap","session","carrier","listen","downlink"})
        assert(log.find(std::string("stage=")+stage)!=std::string::npos);
    assert(log.find("detail="+std::to_string(X509_V_ERR_HOSTNAME_MISMATCH))!=std::string::npos);
    for (const auto& private_text:{std::string("proxy.example.com"),std::string("000102030405060708090a0b0c0d0e0f"),
                                  std::string(43,'A'),std::string(43,'B'),std::string("Authorization")})
        assert(log.find(private_text)==std::string::npos);
    std::cout<<"WEB diagnostics: stages, certificate error codes and credential privacy PASS\n";
    std::cout << "WEB verified DNS fallback/cache, TLS, binary multiplexing, retry/replay, path, cancellation and restart PASS\n";
}
