#include "td/utils/VitaDnsTrace.h"
#include "td/utils/port/IPAddress.h"
#include "td/utils/port/SocketFd.h"
#include <cassert>
#include <cerrno>
#include <netdb.h>
#include <netinet/in.h>
#include <vector>
#include <unistd.h>
#include <sys/socket.h>
#include <string>

struct Event { uint64_t id; unsigned method; int code, detail; int64_t elapsed; };
static std::vector<Event> events;
static uint64_t sequence = 0;
static bool fail_dns = true;
static int system_queries = 0;
static int failed_fd = -1, read_error = ECONNRESET;
static int write_fd = -1, write_error = 0;
static std::string written;
extern "C" ssize_t __real_send(int, const void*, size_t, int);
extern "C" ssize_t __wrap_send(int fd, const void* data, size_t size, int flags) {
    if (fd != write_fd) return __real_send(fd, data, size, flags);
    assert(size <= 16 * 1024);
    if (write_error) { errno = write_error; return -1; }
    written.append(static_cast<const char*>(data), size);
    return size;
}
extern "C" ssize_t __real_read(int, void*, size_t);
extern "C" ssize_t __wrap_read(int fd, void* data, size_t size) {
    if (fd==failed_fd) { errno=read_error;return -1; }
    return __real_read(fd,data,size);
}
extern "C" uint64_t vitagram_dns_begin(unsigned method, unsigned family, unsigned resolver) {
    assert(method <= 4 && (family == 4 || family == 6) && resolver <= 1);
    return ++sequence;
}
extern "C" void vitagram_dns_end(uint64_t id, unsigned method, int code, int detail, int64_t elapsed) {
    assert(id && elapsed >= 0);
    events.push_back({id,method,code,detail,elapsed});
    errno = EACCES; // Diagnostics must not replace the resolver's errno.
}
static std::vector<Event> transport;
extern "C" void vitagram_dns_event(uint64_t id, unsigned phase, int code, int detail, int64_t aux) {
    assert(phase<=5);
    transport.push_back({id,phase,code,detail,aux});
    errno=EACCES;
}
extern "C" int __wrap_getaddrinfo(const char*, const char*, const addrinfo*, addrinfo** out) {
    ++system_queries;
    if (fail_dns) { errno=EHOSTUNREACH;return EAI_AGAIN; }
    static sockaddr_in address{};
    static addrinfo info{};
    address.sin_family=AF_INET;
    address.sin_addr.s_addr=htonl(INADDR_LOOPBACK);
    info.ai_family=AF_INET;info.ai_socktype=SOCK_STREAM;
    info.ai_addr=reinterpret_cast<sockaddr*>(&address);info.ai_addrlen=sizeof(address);
    *out=&info;return 0;
}
extern "C" void __wrap_freeaddrinfo(addrinfo*) {}
int main() {
    td::IPAddress address;
    assert(address.init_host_port("fixture.invalid",443).is_error());
    assert(events.size()==1 && events.back().method==0 && events.back().code==EAI_AGAIN);
    assert(events.back().detail==EHOSTUNREACH && errno==EHOSTUNREACH);
    fail_dns=false;
    assert(address.init_host_port("fixture.invalid",443).is_ok());
    assert(events.size()==2 && events.back().code==0 && events.back().detail==0);
    { td::VitaDnsTrace https;https.start(1,true);https.finish(500,403); }
    assert(events.size()==3 && events.back().method==1 && events.back().detail==403);
    { td::VitaDnsTrace cancelled;cancelled.start(1,false); }
    assert(events.size()==4 && events.back().code==-1);
    fail_dns=true;
    const int before=system_queries;
    assert(address.init_host_port("dns.google",443).is_ok());
    assert(address.get_ip_str()=="8.8.8.8" && address.get_port()==443);
    assert(system_queries==before && events.back().method==2 && events.back().code==0);
    assert(address.init_host_port("dns.google",444).is_error());
    assert(system_queries==before+1);
    const int numeric_before = system_queries;
    assert(address.init_host_port("127.0.0.1",443).is_ok());
    assert(address.get_ip_str()=="127.0.0.1" && address.get_port()==443);
    assert(address.init_host_port("127.0.0.1",0).is_ok());
    assert(address.get_port()==1);
    assert(address.init_host_port("127.0.0.1",65536).is_error());
    assert(system_queries==numeric_before);
    for (size_t i=1;i<events.size();++i) assert(events[i-1].id<events[i].id);
    { td::VitaDnsTrace tls;tls.start(4,false);tls.event(3,1,62,1234);tls.finish(-2,0); }
    assert(transport.size()==1 && transport.back().id==events.back().id);
    assert(transport.back().method==3 && transport.back().code==1 && transport.back().detail==62 && transport.back().elapsed==1234);
    errno=ETIMEDOUT;td::vita_dns_event(0,0,ECONNRESET,3);
    assert(errno==ETIMEDOUT && transport.back().id==0 && transport.back().code==ECONNRESET);
    write_fd=socket(AF_INET,SOCK_STREAM,0); assert(write_fd>=0);
    auto writer=td::SocketFd::from_native_fd(td::NativeFd{write_fd}).move_as_ok();
    std::string payload(512 * 1024, 'x'); payload.back()='z';
    auto first=writer.write(td::Slice(payload)); assert(first.is_ok() && first.ok()==16*1024);
    write_error=ENOBUFS;
    auto pressure=writer.write(td::Slice(payload).substr(first.ok()));
    assert(pressure.is_ok() && pressure.ok()==0 && written.size()==first.ok());
    write_error=0;
    for (size_t offset=first.ok();offset<payload.size();) {
        auto result=writer.write(td::Slice(payload).substr(offset));
        assert(result.is_ok() && result.ok()>0); offset+=result.ok();
    }
    assert(written==payload);
    write_error=ECONNRESET; assert(writer.write("x").is_error());
    writer.close();write_fd=-1;
    failed_fd=socket(AF_INET,SOCK_STREAM,0);assert(failed_fd>=0);
    auto socket=td::SocketFd::from_native_fd(td::NativeFd{failed_fd}).move_as_ok();
    char byte=0;read_error=ENOBUFS;
    auto read_pressure=socket.read(td::MutableSlice(&byte,1)); assert(read_pressure.is_ok() && read_pressure.ok()==0);
    read_error=ECONNRESET;assert(socket.read(td::MutableSlice(&byte,1)).is_error());
    assert(transport.back().id==0 && transport.back().method==0 && transport.back().code==ECONNRESET && transport.back().detail==3);
    socket.close();failed_fd=-1;
}
