#pragma once
#include <atomic>
#include <cstdint>
#include <functional>
#include <string>

namespace update {

struct HttpsResult {
    int status = 0;
    int64_t content_length = -1;
    std::string error; // empty on success
};

struct HttpsResume {
    uint64_t offset = 0, total = 0;
    std::string etag; // strong validator saved with the same artifact, when supplied by the server
    // Called before any final response bytes; 200 means discard the partial first.
    std::function<bool(int, int64_t, const std::string&)> response;
};

// Blocking HTTPS GET over OpenSSL with certificate and host-name verification against `ca_file`.
// Follows up to 5 https redirects. `sink` receives the body of the final 2xx response only;
// Invalid partial responses retry once without Range; the response callback resets the sink.
// returning false aborts. A set `cancel` flag aborts as well.
HttpsResult https_get(const std::string& url, const std::string& ca_file, const std::string& accept,
                      const std::function<bool(const char*, size_t)>& sink, const std::atomic<bool>* cancel,
                      const std::function<void(int64_t)>& on_length = nullptr,
                      const HttpsResume* resume = nullptr);

} // namespace update
