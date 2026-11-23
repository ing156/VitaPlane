#pragma once
#include <atomic>
#include <cstdint>
#include <map>
#include <string>
#include <openssl/ssl.h>
#include <chrono>
#include <mutex>

namespace web_proxy {
struct Reply {
    int status = 0;
    std::string body, error;
    std::map<std::string,std::string> headers;
};
// Worker-only HTTPS, fixed origin, no redirects/cookies/proxy inheritance.
class Http {
public:
    explicit Http(const std::string& ca_file,uint64_t trace=0);
    ~Http();
    Http(const Http&) = delete;
    Http& operator=(const Http&) = delete;
    Reply request(const std::string& host, const std::string& path, const std::string& method,
                  const std::string& token, const std::string& body, const std::string& extra,
                  const std::atomic<bool>& cancel, int timeout_ms = 35000) const;
private:
    SSL_CTX* ctx_ = nullptr;
    uint64_t trace_=0;
    mutable std::mutex dns_mutex_;
    mutable std::string dns_host_,dns_ip_;
    mutable std::chrono::steady_clock::time_point dns_until_;
};
}
