#pragma once
#include <string>
#include <vector>
#include <cstdint>
#include <functional>
#include <mutex>
#include <condition_variable>
#include <pthread.h>

struct DecodedImage {
    unsigned width = 0, height = 0;
    std::vector<uint8_t> rgba;
    const char* error = nullptr; // Static diagnostic code, never a filename.
};
inline DecodedImage image_failure(const char* error) { DecodedImage out; out.error = error; return out; }
struct ImageRequest {
    std::string path;
    unsigned max_side = 1024;
    int mask = 0;
    bool prefetch = false;
    bool operator==(const ImageRequest& other) const {
        return path == other.path && max_side == other.max_side && mask == other.mask;
    }
};
DecodedImage decode_image(const std::string& path, const std::function<bool()>& proceed, unsigned max_side = 1024);

// CPU pixels only. All GPU allocation, upload and destruction remain on UI.
class ImageDecoder {
public:
    using Decode = std::function<DecodedImage(const std::string&, const std::function<bool()>&, unsigned)>;
    explicit ImageDecoder(Decode decode = decode_image);
    ~ImageDecoder();
    void request(const std::vector<ImageRequest>& visible, bool paused);
    bool take(ImageRequest& request, DecodedImage& image);
    void clear();
    void stop();
private:
    void run();
    Decode decode_;
    std::mutex mutex_;
    std::condition_variable wake_;
    pthread_t worker_{};
    bool started_ = false;
    std::vector<ImageRequest> visible_;
    ImageRequest ready_request_;
    DecodedImage ready_;
    uint64_t generation_ = 0;
    bool stopped_ = false, paused_ = true;
};
