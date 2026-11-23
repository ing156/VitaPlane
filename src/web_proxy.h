#pragma once
#include "proxy_config.h"
#include <memory>
#include <cstdint>
#include <string>

// All public calls are nonblocking except destruction. Network I/O stays on workers.
class WebProxy {
public:
    struct Snapshot { int port=0; std::string error; };
    WebProxy();
    ~WebProxy();
    WebProxy(const WebProxy&) = delete;
    WebProxy& operator=(const WebProxy&) = delete;
    void configure(const ProxyConfig& proxy,uint64_t trace=0);
    void cancel();
    Snapshot snapshot() const;
private:
    struct Impl;
    std::unique_ptr<Impl> impl_;
};
