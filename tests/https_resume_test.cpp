#include "https_client.h"
#include <cassert>
#include <string>
int main(int argc, char** argv) {
    assert(argc == 4);
    const std::string base = argv[1], ca = argv[2], mode = argv[3];
    std::string bytes = "abc";
    int responses = 0;
    update::HttpsResume resume;
    resume.offset = 3; resume.total = 6; resume.etag = "\"old\"";
    resume.response = [&](int status, int64_t length, const std::string&) {
        ++responses;
        if (status == 200) bytes.clear();
        assert(length == (status == 200 ? 6 : 3));
        return true;
    };
    auto result = update::https_get(base + "/" + mode, ca, "application/octet-stream",
        [&](const char* data, size_t size) { bytes.append(data, size); return true; }, nullptr, nullptr, &resume);
    if (mode == "cut") { assert(!result.error.empty() && bytes == "abcd"); }
    else if (mode == "duplicate" || mode == "overflow-length") { assert(!result.error.empty() && bytes == "abc" && responses == 0); }
    else { assert(result.error.empty() && bytes == "abcdef" && responses == 1); }
}
