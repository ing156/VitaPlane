#pragma once
#include "tgs_policy.h"
#include "vault.h"
#include <rlottie_capi.h>
#include <zlib.h>
#include <algorithm>
#include <cmath>
#include <cstdint>
#include <functional>
#include <memory>
#include <string>
#include <vector>

// The animation worker and the image worker use the same guarded TGS loader.
class TgsAnimation {
public:
    const char* open(const std::string& path, const std::function<bool()>& proceed, size_t json_limit) {
        std::string packed, data;
        if (!proceed() || !vault::read_file(path,tgs_policy::kMaxPacked,packed)) return "anim.tgs_open";
        z_stream stream{};
        if (inflateInit2(&stream,16+MAX_WBITS)!=Z_OK) return "anim.tgs_invalid";
        stream.next_in=reinterpret_cast<Bytef*>(packed.data()); stream.avail_in=packed.size();
        int status=Z_OK; char block[8192];
        while (status==Z_OK && proceed() && data.size()<=json_limit) {
            stream.next_out=reinterpret_cast<Bytef*>(block); stream.avail_out=sizeof(block);
            status=inflate(&stream,Z_NO_FLUSH);
            data.append(block,sizeof(block)-stream.avail_out);
        }
        inflateEnd(&stream);
        if (!proceed() || status!=Z_STREAM_END || data.empty()) return "anim.tgs_invalid";
        if (data.size()>json_limit || !tgs_policy::acceptable(data)) return "anim.tgs_unsupported";
        animation_.reset(lottie_animation_from_data(data.c_str(),path.c_str(),""));
        if (!animation_) return "anim.tgs_parse";
        frames_=lottie_animation_get_totalframe(animation_.get()); fps_=lottie_animation_get_framerate(animation_.get());
        lottie_animation_get_size(animation_.get(),&source_w_,&source_h_);
        if (!frames_ || frames_>1800 || !std::isfinite(fps_) || fps_<=0 || fps_>120 ||
            !source_w_ || !source_h_ || source_w_>4096 || source_h_>4096) return "anim.tgs_unsupported";
        return nullptr;
    }
    void render(size_t frame, unsigned max_side, std::vector<unsigned char>& rgba, int& width, int& height) {
        const double scale=double(max_side)/std::max(source_w_,source_h_);
        width=std::max(1,int(source_w_*scale)); height=std::max(1,int(source_h_*scale));
        argb_.assign(size_t(width)*height,0); rgba.resize(argb_.size()*4);
        lottie_animation_render(animation_.get(),frame%frames_,argb_.data(),width,height,width*4);
        for (size_t i=0;i<argb_.size();++i) {
            const unsigned pixel=argb_[i], alpha=pixel>>24;
            // rlottie gives premultiplied ARGB; vita2d expects straight RGBA.
            rgba[i*4]=alpha ? std::min(255u,((pixel>>16)&255)*255/alpha) : 0;
            rgba[i*4+1]=alpha ? std::min(255u,((pixel>>8)&255)*255/alpha) : 0;
            rgba[i*4+2]=alpha ? std::min(255u,(pixel&255)*255/alpha) : 0;
            rgba[i*4+3]=alpha;
        }
    }
    size_t frames() const { return frames_; }
    double fps() const { return fps_; }
    const std::vector<uint32_t>& argb() const { return argb_; }
private:
    std::unique_ptr<Lottie_Animation,decltype(&lottie_animation_destroy)> animation_{nullptr,lottie_animation_destroy};
    size_t frames_=0, source_w_=0, source_h_=0;
    double fps_=0;
    std::vector<uint32_t> argb_;
};
