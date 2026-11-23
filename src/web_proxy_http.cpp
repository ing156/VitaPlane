#include "web_proxy_http.h"
#include "web_proxy_protocol.h"
#include "proxy_diagnostics.h"
#include "update_core.h"
#include <netdb.h>
#include <arpa/inet.h>
#include <jansson.h>
#include <sys/socket.h>
#include <sys/select.h>
#include <fcntl.h>
#include <unistd.h>
#include <openssl/x509v3.h>
#include <openssl/err.h>
#include <chrono>
#include <cerrno>
#include <algorithm>
#include <cctype>
#include <cstring>

namespace web_proxy {
namespace {
using proxy_diagnostics::Stage;
using proxy_diagnostics::Endpoint;
Endpoint endpoint(const std::string& host,const std::string& path,const std::string& method) {
    if (host=="dns.google") return Endpoint::Dns;
    if (method=="DELETE") return Endpoint::Delete;
    if (path.find("?bridge=")!=std::string::npos) return Endpoint::Bridge;
    if (path.size()>=3 && path.compare(path.size()-3,3,"/up")==0) return Endpoint::Up;
    if (path.size()>=5 && path.compare(path.size()-5,5,"/down")==0) return Endpoint::Down;
    return Endpoint::Session;
}
struct RequestTrace {
    uint64_t id;Endpoint endpoint;const std::atomic<bool>& cancel;
    Stage phase=Stage::Request;int64_t code=-1,detail=0,aux=0;bool success=false;
    bool verbose() const { return endpoint!=Endpoint::Up && endpoint!=Endpoint::Down; }
    void enter(Stage stage) {
        phase=stage;code=-1;detail=aux=0;
        if (verbose()) proxy_diagnostics::record(stage,id,0,0,endpoint);
    }
    ~RequestTrace() {
        if (cancel) proxy_diagnostics::record(Stage::Cancel,id,0,0,endpoint);
        else if (!success || verbose() || code<200 || code>=300) proxy_diagnostics::record(success?Stage::HttpStatus:phase,id,code,detail,endpoint,aux);
    }
};
}
Http::Http(const std::string& ca_file,uint64_t trace):trace_(trace?trace:proxy_diagnostics::next_id()) {
    ERR_clear_error();
    ctx_ = SSL_CTX_new(TLS_client_method());
    std::string error;
    if (ctx_ && (!SSL_CTX_set_min_proto_version(ctx_,TLS1_2_VERSION) ||
        !update::load_ca_bundle(SSL_CTX_get_cert_store(ctx_),ca_file,error))) {
        SSL_CTX_free(ctx_); ctx_=nullptr;
    }
    if (ctx_) SSL_CTX_set_verify(ctx_,SSL_VERIFY_PEER,nullptr);
    proxy_diagnostics::record(Stage::Ca,trace_,ctx_?0:-1,ctx_?0:ERR_peek_last_error());
}
Http::~Http() { if (ctx_) SSL_CTX_free(ctx_); }
Reply Http::request(const std::string& host, const std::string& path, const std::string& method,
                    const std::string& token, const std::string& body, const std::string& extra,
                    const std::atomic<bool>& cancel, int timeout_ms) const {
    RequestTrace trace{trace_,endpoint(host,path,method),cancel};trace.enter(Stage::Request);
    Reply reply;
    reply.error="proxy.web.failed";
    if (!ctx_) { trace.phase=Stage::Ca;reply.error="proxy.web.tls"; return reply; }
    const auto deadline=std::chrono::steady_clock::now()+std::chrono::milliseconds(timeout_ms);
    struct Connection {
        int fd=-1; SSL* ssl=nullptr;
        ~Connection() { if (ssl) SSL_free(ssl); if (fd>=0) close(fd); }
    } c;
    auto wait = [&](bool write) {
        while (!cancel && std::chrono::steady_clock::now()<deadline) {
            fd_set fds; FD_ZERO(&fds); FD_SET(c.fd,&fds); timeval timeout{0,100000};
            int n=select(c.fd+1,write?nullptr:&fds,write?&fds:nullptr,nullptr,&timeout);
            if (n>0) return true;
            if (n<0 && errno!=EINTR) { trace.code=errno;return false; }
        }
        if (!cancel) { trace.detail=int(trace.phase);trace.phase=Stage::Timeout;trace.code=ETIMEDOUT; }
        return false;
    };
    auto retry = [&](int n) {
        int error=SSL_get_error(c.ssl,n);
        trace.code=error;trace.detail=SSL_get_verify_result(c.ssl);
        trace.aux=error==SSL_ERROR_SYSCALL?errno:ERR_peek_last_error();
        return (error==SSL_ERROR_WANT_READ || error==SSL_ERROR_WANT_WRITE) && wait(error==SSL_ERROR_WANT_WRITE);
    };
    addrinfo hints{},*list=nullptr; hints.ai_family=AF_INET; hints.ai_socktype=SOCK_STREAM;
    if (cancel) return reply;
    trace.enter(Stage::DnsSystem);
    std::string resolved=host;
    if (host=="dns.google") {
        resolved="8.8.8.8";hints.ai_flags=AI_NUMERICHOST;
        trace.enter(Stage::DnsBootstrap);
    }
    {
        std::lock_guard<std::mutex> lock(dns_mutex_);
        if (dns_host_==host && std::chrono::steady_clock::now()<dns_until_) {
            resolved=dns_ip_;hints.ai_flags=AI_NUMERICHOST;
            trace.enter(Stage::DnsCache);
        }
    }
    const int dns_code=getaddrinfo(resolved.c_str(),"443",&hints,&list);
    if (dns_code!=0 || !list) {
        proxy_diagnostics::record(trace.phase,trace_,dns_code,0,trace.endpoint);
        trace.code=dns_code;
        reply.error="proxy.check.dns";
        // Match TDLib's verified DNS fallback; never recurse when the resolver itself fails.
        Address address;
        if (host=="dns.google" || !web_proxy::address(host,address) || !address.path.empty() || cancel) return reply;
        trace.enter(Stage::DnsHttps);
        const auto dns=request("dns.google","/resolve?name="+host+"&type=A","GET","","","",cancel,
                               std::min(timeout_ms,5000));
        trace.code=dns.status;
        if (!dns.error.empty() || dns.status!=200 || dns.body.size()>65536 || cancel) return reply;
        auto* root=json_loadb(dns.body.data(),dns.body.size(),JSON_REJECT_DUPLICATES,nullptr);
        auto* status=json_object_get(root,"Status");
        std::string ip;int ttl=0;
        trace.detail=json_is_integer(status)?json_integer_value(status):-1;
        if (json_is_integer(status) && json_integer_value(status)==0) {
            auto* answers=json_object_get(root,"Answer");size_t i;json_t* answer;
            json_array_foreach(answers,i,answer) {
                auto* value=json_object_get(answer,"data");in_addr numeric{};
                if (json_integer_value(json_object_get(answer,"type"))!=1 || !json_is_string(value) ||
                    json_string_length(value)>15 || std::strlen(json_string_value(value))!=json_string_length(value) ||
                    inet_pton(AF_INET,json_string_value(value),&numeric)!=1) continue;
                ip=json_string_value(value);
                ttl=int(std::clamp<json_int_t>(json_integer_value(json_object_get(answer,"TTL")),0,60));break;
            }
        }
        json_decref(root);
        if (ip.empty()) { trace.aux=1;return reply; }
        hints.ai_flags=AI_NUMERICHOST;
        const int resolved_code=getaddrinfo(ip.c_str(),"443",&hints,&list);
        if (resolved_code!=0 || !list) { trace.aux=resolved_code;return reply; }
        {
            std::lock_guard<std::mutex> lock(dns_mutex_);
            dns_host_=host;dns_ip_=ip;dns_until_=std::chrono::steady_clock::now()+std::chrono::seconds(ttl);
        }
    }
    reply.error="proxy.web.failed";
    trace.enter(Stage::Tcp);
    for (auto* a=list;a && !cancel;a=a->ai_next) {
        c.fd=socket(a->ai_family,a->ai_socktype,a->ai_protocol);
        if (c.fd<0) { trace.code=errno;continue; }
        if (c.fd>=FD_SETSIZE || fcntl(c.fd,F_SETFL,O_NONBLOCK)<0) { trace.code=c.fd>=FD_SETSIZE?EMFILE:errno;close(c.fd);c.fd=-1;continue; }
        const int connected=connect(c.fd,a->ai_addr,a->ai_addrlen);
        int error=0; socklen_t size=sizeof(error);
        if ((connected==0 || (errno==EINPROGRESS && wait(true))) &&
            getsockopt(c.fd,SOL_SOCKET,SO_ERROR,&error,&size)==0 && !error) break;
        if (trace.phase!=Stage::Timeout) trace.code=error?error:errno;
        close(c.fd); c.fd=-1;
    }
    freeaddrinfo(list);
    if (c.fd<0 || cancel) return reply;
    trace.enter(Stage::Tls);
    ERR_clear_error();
    c.ssl=SSL_new(ctx_);
    if (!c.ssl || !SSL_set_fd(c.ssl,c.fd) || !SSL_set_tlsext_host_name(c.ssl,host.c_str()) ||
        !X509_VERIFY_PARAM_set1_host(SSL_get0_param(c.ssl),host.c_str(),0)) { trace.aux=ERR_peek_last_error();reply.error="proxy.web.tls";return reply; }
    for (;;) {
        int n=SSL_connect(c.ssl); if (n==1) break;
        if (!retry(n)) { reply.error="proxy.web.tls";return reply; }
    }
    trace.enter(Stage::HttpIo);
    std::string request=method+" "+path+" HTTP/1.1\r\nHost: "+host+"\r\nConnection: close\r\nAccept-Encoding: identity\r\n";
    if (!token.empty()) request+="Authorization: Bearer "+token+"\r\n";
    if (!body.empty()) request+="Content-Type: application/octet-stream\r\n";
    if (method!="GET") request+="Content-Length: "+std::to_string(body.size())+"\r\n";
    request+=extra+"\r\n"+body;
    for (size_t at=0;at<request.size() && !cancel;) {
        int n=SSL_write(c.ssl,request.data()+at,int(request.size()-at));
        if (n>0) { at+=n;trace.code=0;trace.detail=trace.aux=0; }
        else if (!retry(n)) return reply;
    }
    update::HttpResponse response;
    std::string headers;
    bool headers_done=false;
    auto sink=[&](const char* p,size_t size) {
        if (size>MaxBatch-reply.body.size()) { trace.aux=1;return false; }
        reply.body.append(p,size);return true;
    };
    char buffer[16384];
    while (!cancel && !response.complete()) {
        int n=SSL_read(c.ssl,buffer,sizeof(buffer));
        if (n<=0) {
            if (retry(n)) continue;
            const int error=SSL_get_error(c.ssl,n);
            if ((error==SSL_ERROR_ZERO_RETURN || (error==SSL_ERROR_SYSCALL && !n)) && response.headersDone() &&
                response.bodyIsUntilClose()) break;
            return reply;
        }
        trace.code=0;trace.detail=trace.aux=0;
        if (!headers_done) {
            headers.append(buffer,n);
            const auto end=headers.find("\r\n\r\n");
            if (end!=std::string::npos) { headers.resize(end+2);headers_done=true; }
            if (headers.size()>32768) { trace.code=-2;return reply; }
        }
        if (!response.feed(buffer,n,sink)) { trace.code=-3;return reply; }
    }
    if (cancel || !headers_done) { trace.code=-5;return reply; }
    size_t at=headers.find("\r\n")+2;
    while (at<headers.size()) {
        const auto end=headers.find("\r\n",at),colon=headers.find(':',at);
        if (end==std::string::npos || colon==std::string::npos || colon>=end) { trace.code=-4;return reply; }
        auto name=headers.substr(at,colon-at), value=headers.substr(colon+1,end-colon-1);
        for (auto& ch:name) ch=std::tolower(static_cast<unsigned char>(ch));
        while (!value.empty() && (value.front()==' ' || value.front()=='\t')) value.erase(0,1);
        while (!value.empty() && (value.back()==' ' || value.back()=='\t')) value.pop_back();
        if (!reply.headers.emplace(name,value).second && name.compare(0,2,"x-")==0) { trace.code=-4;return reply; }
        at=end+2;
    }
    reply.status=response.status();reply.error.clear();trace.success=true;trace.code=reply.status;trace.detail=trace.aux=0;return reply;
}
}
