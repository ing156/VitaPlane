#include "web_proxy.h"
#include "web_proxy_http.h"
#include "web_proxy_protocol.h"
#include "proxy_diagnostics.h"
#include <atomic>
#include <condition_variable>
#include <mutex>
#include <pthread.h>
#include <map>
#include <deque>
#include <chrono>
#include <cerrno>
#include <cstring>
#include <netinet/in.h>
#include <sys/socket.h>
#include <sys/select.h>
#include <fcntl.h>
#include <unistd.h>

namespace {
constexpr size_t MaxStreams=8, MaxPending=8u<<20, MaxItems=4096, UpBatch=128u<<10;
constexpr const char* CaFile="app0:certs/cacert.pem";
int start_tls_thread(pthread_t& thread, void* (*entry)(void*), void* data) {
    pthread_attr_t attr;
    int rc=pthread_attr_init(&attr);
    if (rc) return rc;
    // CA parsing, TLS and HTTP buffers exceed Vita's default 32 KiB stack.
    rc=pthread_attr_setstacksize(&attr,256*1024);
    if (!rc) rc=pthread_create(&thread,&attr,entry,data);
    pthread_attr_destroy(&attr);
    return rc;
}
bool nonblocking(int fd) { return fd>=0 && fd<FD_SETSIZE && fcntl(fd,F_SETFL,O_NONBLOCK)>=0; }
bool transient() { return errno==EINTR || errno==EAGAIN || errno==EWOULDBLOCK; }
}
struct WebProxy::Impl {
    mutable std::mutex mutex;
    std::condition_variable wake;
    pthread_t worker{};
    bool worker_started=false;
    std::atomic<bool> cancelled{false};
    bool stopping=false, pending=false;
    ProxyConfig config;
    uint64_t trace=0,pending_trace=0;
    Snapshot state;
    struct Stream {
        int fd=-1;
        uint32_t tx=web_proxy::Window, rx=web_proxy::Window;
        std::deque<std::string> received;
        size_t offset=0;
        bool closed=false;
    };
    std::map<uint32_t,Stream> streams;
    uint32_t next_id=0;
    size_t queued_bytes=0,queued_items=0;
    std::string controls;
    bool welcomed=false;
    void fail(const std::string& error) {
        std::lock_guard<std::mutex> lock(mutex);
        if (!cancelled) {
            const int reason=error=="proxy.web.invalid"?10:error=="proxy.web.tls"?11:
                error=="proxy.check.dns"?12:error=="proxy.web.unsupported"?13:14;
            proxy_diagnostics::record(proxy_diagnostics::Stage::Failed,trace,reason);
            state.error=error;state.port=0;cancelled=true;
        }
    }
    bool pause(int milliseconds) {
        std::unique_lock<std::mutex> lock(mutex);
        wake.wait_for(lock,std::chrono::milliseconds(milliseconds),[&] { return cancelled.load(); });
        return !cancelled;
    }
    web_proxy::Reply request(const web_proxy::Http& http,const web_proxy::Address& address,
                            const std::string& method,const std::string& endpoint,
                            const std::string& token,const std::string& body,const std::string& extra={}) {
        const auto deadline=std::chrono::steady_clock::now()+std::chrono::seconds(90);
        web_proxy::Reply reply;
        const auto path="/"+(address.path.empty()?"":address.path+"/")+endpoint;
        unsigned attempts=0;
        do {
            reply=http.request(address.host,path,method,token,body,extra,cancelled);
            if ((reply.error.empty() && reply.status!=503) || (reply.error=="proxy.web.tls" || reply.error=="proxy.check.dns")) return reply;
            if (cancelled || (!reply.error.empty() && ++attempts>=8)) break;
            proxy_diagnostics::record(proxy_diagnostics::Stage::Retry,trace,reply.status,attempts);
            if (!pause(reply.status==503 ? 1000 : std::min(5000,200<<std::min(attempts,4u)))) break;
        } while (std::chrono::steady_clock::now()<deadline);
        return reply;
    }
    bool incoming(const std::string& body) {
        using namespace web_proxy;
        std::vector<Frame> batch;
        if (!frames(body,batch)) return false;
        std::unique_lock<std::mutex> lock(mutex);
        for (auto& f:batch) {
            if (f.type==0x11) { if (welcomed) return false;welcomed=true;continue; }
            if (!welcomed) return false;
            if (f.type==5) {
                if (controls.size()+f.payload.size()+8>UpBatch) return false;
                controls+=frame(6,0,f.payload);continue;
            }
            if (f.type!=2 && f.type!=3 && f.type!=4) return false;
            if (f.type==2) {
                while (!cancelled && (f.payload.size()+256>MaxPending-queued_bytes || queued_items>=MaxItems))
                    wake.wait_for(lock,std::chrono::milliseconds(100));
                if (cancelled) return false;
            }
            const auto found=streams.find(f.stream);
            if (found==streams.end()) { if (f.stream>next_id) return false;continue; }
            auto& s=found->second;
            if (s.closed) continue;
            if (f.type==3) s.closed=true;
            else if (f.type==4) {
                const uint32_t delta=number(f.payload.data());
                if (delta>Window-s.tx) return false;
                s.tx+=delta;
            } else {
                if (f.payload.size()>65536 || f.payload.size()>s.rx || f.payload.size()+256>MaxPending-queued_bytes || queued_items>=MaxItems) return false;
                s.rx-=f.payload.size();queued_bytes+=f.payload.size()+256;++queued_items;
                s.received.push_back(std::move(f.payload));
            }
        }
        return true;
    }
    void down(const web_proxy::Http& http,const web_proxy::Address& address,const std::string& token) {
        uint64_t cursor=0;
        while (!cancelled) {
            auto r=request(http,address,"POST","api/v1/down",token,"","X-Down-Cursor: "+std::to_string(cursor)+"\r\n");
            if (cancelled) break;
            const auto value=r.headers["x-down-cursor"];
            if (!r.error.empty() || (r.status!=200 && r.status!=204) ||
                value!=std::to_string(cursor+(r.status==200)) ||
                (r.status==200 ? !incoming(r.body) : !r.body.empty())) {
                proxy_diagnostics::record(proxy_diagnostics::Stage::Downlink,trace,r.status,
                    value!=std::to_string(cursor+(r.status==200))?1:2,proxy_diagnostics::Endpoint::Down);
                fail(r.error.empty()?"proxy.web.failed":r.error);break;
            }
            if (r.status==200) ++cursor;
        }
    }
    void session(const ProxyConfig& config) {
        using namespace web_proxy;
        Address a;std::vector<uint8_t> key;bool marked=false;
        if (!address(config.server,a) || !secret(config.secret,key,marked) || (marked && a.path.empty()) || config.port!=443) { fail("proxy.web.invalid");return; }
        Http http(CaFile,trace);
        auto bridge=request(http,a,"GET","?bridge="+capability(a,key),"","");
        std::string bootstrap_token;
        const bool bridge_valid=bridge.error.empty() && bridge.status==200 && bootstrap(bridge.body,bootstrap_token);
        proxy_diagnostics::record(proxy_diagnostics::Stage::Bootstrap,trace,bridge.status,bridge_valid);
        if (!bridge_valid) {
            fail(bridge.error.empty()?"proxy.web.failed":bridge.error);return;
        }
        auto created=request(http,a,"POST","api/v1/session",bootstrap_token,frame(0x10,0,std::string(1,'\1')));
        const std::string token=created.headers["x-session-token"];
        const bool session_valid=created.error.empty() && created.status==200 && bearer(token);
        proxy_diagnostics::record(proxy_diagnostics::Stage::Session,trace,created.status,session_valid);
        if (!session_valid) {
            fail(created.error.empty()?"proxy.web.failed":created.error);return;
        }
        struct DeleteSession {
            const Http& http; const Address& address; const std::string& token;
            ~DeleteSession() {
                const std::atomic<bool> live{false};
                http.request(address.host,"/"+(address.path.empty()?"":address.path+"/")+"api/v1/session",
                    "DELETE",token,"","",live,1000);
            }
        } cleanup{http,a,token};
        // shortcut: native HTTPS carrier only; add the lane/WebSocket carriers when needed on Vita.
        const auto& carrier=created.headers["x-carrier-mode"];
        proxy_diagnostics::record(proxy_diagnostics::Stage::Carrier,trace,carrier=="https"?1:carrier=="https-lanes"?2:carrier=="websocket"?3:0);
        if (carrier!="https") { fail("proxy.web.unsupported");return; }
        if (created.body!=frame(0x11,0) || !incoming(created.body)) { fail("proxy.web.failed");return; }
        int listener=socket(AF_INET,SOCK_STREAM,0);
        sockaddr_in local{}; local.sin_family=AF_INET;local.sin_addr.s_addr=htonl(INADDR_LOOPBACK);
        socklen_t length=sizeof(local);
        if (!nonblocking(listener) || bind(listener,reinterpret_cast<sockaddr*>(&local),length)<0 ||
            listen(listener,8)<0 || getsockname(listener,reinterpret_cast<sockaddr*>(&local),&length)<0) {
            proxy_diagnostics::record(proxy_diagnostics::Stage::Listen,trace,errno);
            if (listener>=0) close(listener);
            fail("proxy.web.failed");return;
        }
        struct Downlink { Impl* self; const Http& http; const Address& address; const std::string& token; } downlink{this,http,a,token};
        pthread_t reader{};
        const int reader_error=start_tls_thread(reader,[](void* data) -> void* {
            auto& d=*static_cast<Downlink*>(data);
            d.self->down(d.http,d.address,d.token);return nullptr;
        },&downlink);
        proxy_diagnostics::record(proxy_diagnostics::Stage::Downlink,trace,reader_error);
        if (reader_error) { close(listener);fail("proxy.web.failed");return; }
        proxy_diagnostics::record(proxy_diagnostics::Stage::Listen,trace,0,ntohs(local.sin_port));
        { std::lock_guard<std::mutex> lock(mutex); if (!cancelled) state.port=ntohs(local.sin_port); }
        uint64_t sequence=0;
        while (!cancelled) {
            fd_set readset,writeset;FD_ZERO(&readset);FD_ZERO(&writeset);FD_SET(listener,&readset);int last=listener;
            {
                std::lock_guard<std::mutex> lock(mutex);
                for (auto& entry:streams) {
                    auto& s=entry.second;
                    if (s.tx) FD_SET(s.fd,&readset);
                    if (!s.received.empty() || s.closed) FD_SET(s.fd,&writeset);
                    last=std::max(last,s.fd);
                }
            }
            timeval timeout{0,20000};
            int selected=select(last+1,&readset,&writeset,nullptr,&timeout);
            if (selected<0) {
                if (errno==EINTR) continue;
                proxy_diagnostics::record(proxy_diagnostics::Stage::Listen,trace,errno);
                fail("proxy.web.failed");break;
            }
            std::string batch;
            {
                std::lock_guard<std::mutex> lock(mutex);
                batch.swap(controls);
                if (FD_ISSET(listener,&readset)) {
                    const int fd=accept(listener,nullptr,nullptr);
                    if (fd>=0) {
                        if (streams.size()>=MaxStreams || next_id==0xffffff || !nonblocking(fd)) close(fd);
                        else { auto& s=streams[++next_id];s.fd=fd;batch+=frame(1,next_id); }
                    }
                }
                for (auto it=streams.begin();it!=streams.end();) {
                    auto& s=it->second;const auto id=it->first;
                    if (!s.closed && FD_ISSET(s.fd,&writeset)) {
                        uint32_t delivered=0;
                        while (!s.received.empty()) {
                            auto& data=s.received.front();
                            const int n=send(s.fd,data.data()+s.offset,data.size()-s.offset,0);
                            if (n<=0) { if (n<0 && transient()) break;s.closed=true;break; }
                            s.offset+=n;delivered+=n;s.rx+=n;queued_bytes-=n;
                            if (s.offset==data.size()) { s.received.pop_front();s.offset=0;queued_bytes-=256;--queued_items; }
                        }
                        if (delivered) batch+=frame(4,id,number(delivered));
                    }
                    if (!s.closed && s.tx && FD_ISSET(s.fd,&readset) && batch.size()+16392<=UpBatch) {
                        char data[16384];
                        const int n=recv(s.fd,data,std::min<size_t>(sizeof(data),s.tx),0);
                        if (n>0) { s.tx-=n;batch+=frame(2,id,std::string(data,n)); }
                        else if (n==0 || !transient()) s.closed=true;
                    }
                    if (s.closed) {
                        bool first=true;
                        for (const auto& data:s.received) {
                            queued_bytes-=data.size()-(first?s.offset:0)+256;--queued_items;first=false;
                        }
                        close(s.fd);batch+=frame(3,id);it=streams.erase(it);
                    } else ++it;
                }
            }
            wake.notify_all(); // Resume downlink admission after local sockets drain.
            if (!batch.empty()) {
                auto r=request(http,a,"POST","api/v1/up",token,batch,"X-Up-Seq: "+std::to_string(sequence+1)+"\r\n");
                if (!cancelled && (!r.error.empty() || r.status!=204 || r.headers["x-up-ack"]!=std::to_string(sequence+1))) {
                    proxy_diagnostics::record(proxy_diagnostics::Stage::HttpStatus,trace,r.status,
                        r.headers["x-up-ack"]==std::to_string(sequence+1),proxy_diagnostics::Endpoint::Up);
                    fail(r.error.empty()?"proxy.web.failed":r.error);break;
                }
                ++sequence;
            }
        }
        close(listener);cancelled=true;pthread_join(reader,nullptr);
    }
    void run() {
        for (;;) {
            ProxyConfig next;
            {
                std::unique_lock<std::mutex> lock(mutex);
                wake.wait(lock,[&] { return pending || stopping; });
                if (stopping) return;
                next=config;trace=pending_trace;pending=false;cancelled=false;
            }
            session(next);
            std::lock_guard<std::mutex> lock(mutex);
            for (auto& entry:streams) close(entry.second.fd);
            streams.clear();next_id=0;queued_bytes=queued_items=0;controls.clear();welcomed=false;
            if (!pending) state.port=0;
        }
    }
};
WebProxy::WebProxy():impl_(new Impl) {}
WebProxy::~WebProxy() {
    { std::lock_guard<std::mutex> lock(impl_->mutex);impl_->stopping=true;impl_->cancelled=true; }
    impl_->wake.notify_one();
    if (impl_->worker_started) pthread_join(impl_->worker,nullptr);
}
void WebProxy::configure(const ProxyConfig& proxy,uint64_t trace) {
    std::lock_guard<std::mutex> lock(impl_->mutex);
    impl_->cancelled=true;impl_->config=proxy;impl_->state={};impl_->pending=true;
    impl_->pending_trace=trace?trace:proxy_diagnostics::next_id();
    if (!impl_->worker_started) {
        const int worker_error=start_tls_thread(impl_->worker,[](void* data) -> void* {
            static_cast<Impl*>(data)->run();return nullptr;
        },impl_.get());
        impl_->worker_started=worker_error==0;
        if (!impl_->worker_started) {
            proxy_diagnostics::record(proxy_diagnostics::Stage::Failed,impl_->pending_trace,15,worker_error);
            impl_->pending=false;impl_->state.error="proxy.web.failed";
        }
    }
    impl_->wake.notify_one();
}
void WebProxy::cancel() {
    std::lock_guard<std::mutex> lock(impl_->mutex);
    if (!impl_->cancelled && (impl_->pending || impl_->state.port))
        proxy_diagnostics::record(proxy_diagnostics::Stage::Cancel,impl_->pending?impl_->pending_trace:impl_->trace);
    impl_->cancelled=true;impl_->pending=false;impl_->state={};impl_->wake.notify_one();
}
WebProxy::Snapshot WebProxy::snapshot() const { std::lock_guard<std::mutex> lock(impl_->mutex);return impl_->state; }
