#include "image_decoder.h"
#include "image_retry.h"
#include <png.h>
#include <cstdio>
#include <jpeglib.h>
#include <webp/encode.h>
#include <cassert>
#include <atomic>
#include <chrono>
#include <thread>
#include <fstream>
#include <cstdlib>
#ifdef VITAGRAM_RLOTTIE
#include <zlib.h>
#endif

using namespace std::chrono_literals;
template<class F> void wait_for(F predicate) {
    for (int i=0; i<2000 && !predicate(); ++i) std::this_thread::sleep_for(1ms);
    assert(predicate());
}
int main() {
#ifdef VITAGRAM_RLOTTIE
    std::ifstream tgs_json(VITAGRAM_TEST_TGS_JSON);
    const std::string model((std::istreambuf_iterator<char>(tgs_json)),{}); assert(!model.empty());
    auto write_tgs=[](const char* path,const std::string& data) {
        gzFile file=gzopen(path,"wb"); assert(file);
        assert(gzwrite(file,data.data(),data.size())==int(data.size())); assert(gzclose(file)==Z_OK);
    };
    write_tgs("preview.tgs",model);
    auto preview=decode_image("tgs-thumb:preview.tgs",[]{return true;},64);
    assert(preview.width==64 && preview.height==64 && preview.rgba.size()==64*64*4);
    const size_t center=(32*64+32)*4;
    assert(preview.rgba[center]==255 && preview.rgba[center+3]==255 && preview.rgba[3]==0);
    assert(decode_image("tgs-thumb:preview.tgs",[]{return false;},64).rgba.empty());
    write_tgs("oversized.tgs",std::string(256*1024+1,' '));
    assert(decode_image("tgs-thumb:oversized.tgs",[]{return true;},64).rgba.empty());
    write_tgs("embedded.tgs",R"({"w":32,"h":32,"fr":30,"ip":0,"op":30,"assets":[{"p":"data:image/png;base64,AAAA"}]})");
    assert(decode_image("tgs-thumb:embedded.tgs",[]{return true;},64).rgba.empty());
#endif
    ImageRetry retries;
    assert(retries.ready("temporary.jpg", 0));
    assert(retries.fail("temporary.jpg", 100) == 1);
    assert(!retries.ready("temporary.jpg", 1000099) && retries.ready("temporary.jpg", 1000100));
    assert(retries.fail("temporary.jpg", 1000100) == 2);
    assert(!retries.ready("temporary.jpg", 4000099) && retries.ready("temporary.jpg", 4000100));
    assert(retries.fail("temporary.jpg", 4000100) == 3);
    assert(retries.fail("temporary.jpg", 14000100) == 4);
    assert(!retries.ready("temporary.jpg", 44000099) && retries.ready("temporary.jpg", 44000100));
    assert(retries.fail("temporary.jpg", 44000100) == 4);
    retries.forget("temporary.jpg"); assert(retries.ready("temporary.jpg", 0));
    retries.fail("another.jpg", 0); retries.clear(); assert(retries.ready("another.jpg", 0));
    retries.fail("variants.jpg",0,1024,0);
    assert(!retries.ready("variants.jpg",0,1024,0));
    assert(retries.ready("variants.jpg",0,128,0) && retries.ready("variants.jpg",0,1024,3));
    retries.fail("variants.jpg",0,128,1);
    retries.forget("variants.jpg",1024,0);
    assert(retries.ready("variants.jpg",0,1024,0) && !retries.ready("variants.jpg",0,128,1));
    retries.forget("variants.jpg");assert(retries.ready("variants.jpg",0,128,1));
    const uint8_t pixels[]={255,0,0,255,0,255,0,255,0,0,255,255,255,255,255,255};
    png_image png{}; png.version=PNG_IMAGE_VERSION; png.width=2; png.height=2; png.format=PNG_FORMAT_RGBA;
    assert(png_image_write_to_file(&png,"image.png",0,pixels,0,nullptr));
    jpeg_compress_struct jpeg{}; jpeg_error_mgr error{}; jpeg.err=jpeg_std_error(&error);
    jpeg_create_compress(&jpeg); FILE* file=fopen("image.jpg","wb"); assert(file); jpeg_stdio_dest(&jpeg,file);
    jpeg.image_width=2; jpeg.image_height=2; jpeg.input_components=3; jpeg.in_color_space=JCS_RGB;
    jpeg_set_defaults(&jpeg); jpeg_start_compress(&jpeg,TRUE);
    uint8_t row0[]={255,0,0,0,255,0},row1[]={0,0,255,255,255,255};
    JSAMPROW row=row0; jpeg_write_scanlines(&jpeg,&row,1);row=row1;jpeg_write_scanlines(&jpeg,&row,1);
    jpeg_finish_compress(&jpeg); jpeg_destroy_compress(&jpeg); fclose(file);
    std::ifstream jpeg_input("image.jpg", std::ios::binary);
    const std::string raw((std::istreambuf_iterator<char>(jpeg_input)), std::istreambuf_iterator<char>());
    const char* alphabet = "ABCDEFGHIJKLMNOPQRSTUVWXYZabcdefghijklmnopqrstuvwxyz0123456789+/";
    std::string mini_encoded;
    for (size_t i = 0; i < raw.size(); i += 3) {
        unsigned bits = (unsigned char)raw[i] << 16;
        if (i + 1 < raw.size()) bits |= (unsigned char)raw[i + 1] << 8;
        if (i + 2 < raw.size()) bits |= (unsigned char)raw[i + 2];
        mini_encoded += alphabet[(bits >> 18) & 63]; mini_encoded += alphabet[(bits >> 12) & 63];
        mini_encoded += i + 1 < raw.size() ? alphabet[(bits >> 6) & 63] : '=';
        mini_encoded += i + 2 < raw.size() ? alphabet[bits & 63] : '=';
    }
    auto mini = decode_image("inline-jpeg:" + mini_encoded, [] { return true; });
    assert(mini.width == 2 && mini.height == 2 && mini.rgba.size() == 16);
    assert(decode_image("inline-jpeg:broken", [] { return true; }).rgba.empty());

    uint8_t* encoded=nullptr;size_t bytes=WebPEncodeLosslessRGBA(pixels,2,2,8,&encoded);assert(bytes);
    {std::ofstream out("image.webp",std::ios::binary);out.write(reinterpret_cast<char*>(encoded),bytes);}WebPFree(encoded);
    for (auto path:{"image.png","image.jpg","image.webp"}) {
        auto image=decode_image(path,[]{return true;});assert(image.width==2 && image.height==2 && image.rgba.size()==16);
        for (size_t i=3;i<16;i+=4)assert(image.rgba[i]==255);
        if (std::string(path)!="image.jpg") assert(image.rgba==std::vector<uint8_t>(pixels,pixels+16));
        assert(decode_image(path,[]{return false;}).rgba.empty());
    }
    jpeg={};jpeg.err=jpeg_std_error(&error);jpeg_create_compress(&jpeg);
    file=fopen("large.jpg","wb");assert(file);jpeg_stdio_dest(&jpeg,file);
    jpeg.image_width=4096;jpeg.image_height=2048;jpeg.input_components=3;jpeg.in_color_space=JCS_RGB;
    jpeg_set_defaults(&jpeg);jpeg_start_compress(&jpeg,TRUE);
    std::vector<uint8_t> jpeg_row(4096*3,255);row=jpeg_row.data();
    while (jpeg.next_scanline<jpeg.image_height) jpeg_write_scanlines(&jpeg,&row,1);
    jpeg_finish_compress(&jpeg);jpeg_destroy_compress(&jpeg);fclose(file);
    auto large=decode_image("large.jpg",[]{return true;});
    assert(large.width==1024 && large.height==512 && large.rgba.size()==1024*512*4);
    {std::ofstream out("broken.jpg",std::ios::binary);out.put(char(0xff));out.put(char(0xd8));out << "corrupt";}
    assert(decode_image("broken.jpg",[]{return true;}).rgba.empty());
    assert(decode_image("missing",[]{return true;}).rgba.empty());
    assert(std::string(decode_image("missing",[]{return true;}).error) == "file_open");
    retries.fail("later.jpg", 0);
    { std::ofstream repaired("later.jpg", std::ios::binary); repaired.write(raw.data(), raw.size()); }
    assert(!retries.ready("later.jpg", 999999) && retries.ready("later.jpg", 1000000));
    assert(decode_image("later.jpg", [] { return true; }).width == 2);
    retries.forget("later.jpg");
    {std::ofstream out("huge",std::ios::binary);out.seekp(16*1024*1024);out.put('x');}
    assert(decode_image("huge",[]{return true;}).rgba.empty());
    std::vector<uint8_t> wide(2048*2*4,255);
    png.width=2048;png.height=2;
    assert(png_image_write_to_file(&png,"wide.png",0,wide.data(),0,nullptr));
    auto scaled=decode_image("wide.png",[]{return true;});
    assert(scaled.width==1024 && scaled.height==1 && scaled.rgba.size()==4096);
    assert(scaled.rgba.front()==255 && scaled.rgba.back()==255);
    std::atomic<int> entered{0};std::atomic<bool> release{false};
    ImageDecoder worker([&](const std::string&,const std::function<bool()>& proceed,unsigned){
        ++entered;
        while (!release && proceed()) std::this_thread::sleep_for(1ms);
        return proceed() ? DecodedImage{1,1,{255,0,0,255}} : DecodedImage{};
    });
    worker.request({{"old"}},false);wait_for([&]{return entered.load()==1;});
    worker.request({{"new"}},false);release=true;
    ImageRequest request;DecodedImage result;
    wait_for([&]{return worker.take(request,result) || request.path=="new";});assert(request.path=="new" && result.rgba.size()==4);
    release=false; worker.request({{"logout"}},false);wait_for([&]{return entered.load()>=3;});
    worker.clear();release=true;std::this_thread::sleep_for(10ms);assert(!worker.take(request,result));
    release=false; worker.request({{"pause"}},true);int before=entered;std::this_thread::sleep_for(10ms);assert(entered==before);
    worker.request({{"pause"}},false);wait_for([&]{return entered.load()>before;});worker.request({{"pause"}},true);
    worker.stop();assert(!worker.take(request,result));
    std::atomic<int> generation_calls{0};std::atomic<bool> gate{false};
    ImageDecoder generation_worker([&](const std::string&,const std::function<bool()>&,unsigned){
        int call=++generation_calls;
        while (!gate) std::this_thread::sleep_for(1ms);
        return DecodedImage{1,1,{uint8_t(call),0,0,255}};
    });
    generation_worker.request({{"same"}},false);wait_for([&]{return generation_calls==1;});
    generation_worker.clear();generation_worker.request({{"same"}},false);gate=true;
    request={};
    wait_for([&]{return generation_worker.take(request,result) || !request.path.empty();});
    assert(request.path=="same" && result.rgba.front()==2);
    generation_worker.stop();
    for (unsigned side : {128u,384u,1024u}) {
        auto sized = decode_image("large.jpg",[]{return true;},side);
        assert(sized.width == side && sized.height == side/2);
        assert(sized.rgba.size() == size_t(side)*side/2*4);
        for (size_t i=3;i<sized.rgba.size();i+=4) assert(sized.rgba[i]==255);
        sized = decode_image("wide.png",[]{return true;},side);
        assert(sized.width == side && sized.height == 1);
    }
    std::vector<uint8_t> webp_pixels(800*400*4,255);
    webp_pixels[3]=0;
    bytes=WebPEncodeLosslessRGBA(webp_pixels.data(),800,400,800*4,&encoded);assert(bytes);
    {std::ofstream out("large.webp",std::ios::binary);out.write(reinterpret_cast<char*>(encoded),bytes);}WebPFree(encoded);
    auto webp_small=decode_image("large.webp",[]{return true;},128);
    assert(webp_small.width==128 && webp_small.height==64 && webp_small.rgba.size()==128*64*4);
    auto webp_full=decode_image("large.webp",[]{return true;},1024);
    assert(webp_full.width==800 && webp_full.height==400 && webp_full.rgba[3]==0);
    assert(decode_image("large.jpg",[]{return true;},0).width==1);
    assert(decode_image("large.jpg",[]{return true;},4096).width==1024);
    assert(!(ImageRequest{"same",128,0} == ImageRequest{"same",384,0}));
    assert(!(ImageRequest{"same",128,0} == ImageRequest{"same",128,3}));
    std::atomic<int> small_started{0};
    ImageDecoder size_worker([&](const std::string&,const std::function<bool()>& proceed,unsigned side){
        if (side==128) { ++small_started; while(proceed()) std::this_thread::sleep_for(1ms); return DecodedImage{}; }
        return DecodedImage{1,1,{255,0,0,255}};
    });
    size_worker.request({{"same",128}},false);wait_for([&]{return small_started==1;});
    size_worker.request({{"same",384}},false);request={};
    wait_for([&]{return size_worker.take(request,result) || !request.path.empty();});
    assert(request.max_side==384 && result.rgba.size()==4);size_worker.stop();
    std::atomic<int> neighbor_started{0};
    ImageDecoder priority_worker([&](const std::string& name,const std::function<bool()>& proceed,unsigned){
        if (name=="neighbor" && ++neighbor_started==1) {
            while(proceed()) std::this_thread::sleep_for(1ms);
            return DecodedImage{};
        }
        return DecodedImage{1,1,{255,0,0,255}};
    });
    priority_worker.request({{"neighbor",128,1,true}},false);
    wait_for([&]{return neighbor_started==1;});
    priority_worker.request({{"neighbor",128,1,true},{"visible",128,1}},false);request={};
    wait_for([&]{return priority_worker.take(request,result) || !request.path.empty();});
    assert(request.path=="visible" && result.rgba.size()==4);request={};
    wait_for([&]{return priority_worker.take(request,result) || !request.path.empty();});
    assert(request.path=="neighbor" && neighbor_started==2 && result.rgba.size()==4);
    priority_worker.stop();
    std::puts("Images: PNG/JPEG/WebP pixels, damaged/oversized files, stale screen, clear, pause and stop passed");
}
