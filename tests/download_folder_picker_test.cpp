#include "download_folder_picker.h"
#include <psp2/io/dirent.h>
#include <psp2/io/stat.h>
#include <cassert>
#include <chrono>
#include <condition_variable>
#include <cstring>
#include <map>
#include <thread>
#include <cstdio>
#include <cerrno>

static bool fail_thread = false;
extern "C" int __real_pthread_create(pthread_t*,const pthread_attr_t*,void*(*)(void*),void*);
extern "C" int __wrap_pthread_create(pthread_t* thread,const pthread_attr_t* attr,void*(*entry)(void*),void* arg) {
    size_t size = 0; assert(pthread_attr_getstacksize(attr,&size)==0 && size>=256*1024);
    return fail_thread ? EAGAIN : __real_pthread_create(thread,attr,entry,arg);
}
static std::map<std::string,std::vector<SceIoDirent>> directories;
static std::string current_directory;
static size_t row = 0;
static std::mutex gate;
static std::condition_variable condition;
static bool block_open = false, entered = false, read_error = false;
static const auto ui_thread = std::this_thread::get_id();
SceUID sceIoDopen(const char* path) {
    assert(std::this_thread::get_id() != ui_thread);
    std::unique_lock<std::mutex> lock(gate);
    if (std::string(path) == "uma0:blocked") {
        entered = true; condition.notify_all();
        condition.wait(lock, [] { return !block_open; });
    }
    current_directory = path; row = 0;
    return directories.count(path) ? 1 : -1;
}
int sceIoDread(SceUID,SceIoDirent* result) {
    assert(std::this_thread::get_id() != ui_thread);
    if (read_error) return -1;
    const auto& entries = directories.at(current_directory);
    if (row == entries.size()) return 0;
    *result = entries[row++]; return 1;
}
int sceIoDclose(SceUID) { return 0; }
int sceIoMkdir(const char* path,SceMode) { assert(std::this_thread::get_id() != ui_thread); directories[path] = {}; return 0; }
static SceIoDirent entry(const char* name,bool dir) {
    SceIoDirent result{}; std::strncpy(result.d_name,name,sizeof(result.d_name)-1);
    result.d_stat.st_mode = dir ? SCE_S_IFDIR : SCE_S_IFREG; return result;
}
static void ready(DownloadFolderPicker& picker) {
    const auto deadline = std::chrono::steady_clock::now()+std::chrono::seconds(5);
    while (picker.loading) { picker.poll(); assert(std::chrono::steady_clock::now()<deadline); std::this_thread::yield(); }
}
int main(int argc,char** argv) {
    assert(argc == 2); i18n::init(argv[1],"en");
    directories["ux0:/"] = {}; directories["ux0:downloads"] = {};
    directories["ux0:default"] = {entry("z.txt",false),entry("B",true),entry("a.txt",false),entry("A",true),entry(".",true),entry("..",true)};
    directories["ux0:default/A"] = {}; directories["uma0:last"] = {entry("only-file.txt",false)};
    DownloadFolderPicker picker;
    fail_thread=true;picker.open("uma0:last","ux0:default");assert(!picker.loading && !picker.error.empty());fail_thread=false;
    picker.open("uma0:last","ux0:default");ready(picker);assert(picker.path=="uma0:last" && picker.entries.size()==1 && !picker.entries[0].dir);
    picker.enter();assert(picker.path=="uma0:last" && !picker.loading); // Files are visible, never entered as folders.
    picker.open("uma0:removed","ux0:default");ready(picker);
    assert(picker.path=="ux0:default" && picker.entries.size()==4 && picker.entries[0].name=="A" && picker.entries[1].name=="B");
    assert(!picker.entries[2].dir && picker.entries[2].name=="a.txt");
    picker.enter();ready(picker);assert(picker.path=="ux0:default/A");
    assert(!picker.create("../bad") && picker.create("Новая папка"));ready(picker);assert(picker.path=="ux0:default/A/Новая папка");
    picker.up();ready(picker);assert(picker.path=="ux0:default/A");
    picker.open("bad:../path","unavailable:");ready(picker);assert(picker.path=="ux0:downloads");
    read_error=true;picker.reload();ready(picker);assert(!picker.error.empty());read_error=false;
    directories["ux0:many"].assign(5002,entry("file.txt",false));
    picker.open("ux0:many");ready(picker);assert(picker.entries.size()==5000);
    picker.selected=4999;
    for(int y=0;y<544;++y)for(int x=0;x<960;++x) {
        const int hit=picker.hit(x,y), local=y-DownloadFolderPicker::list_top;
        const bool expected=x>=32 && x<928 && local>=0 && local<8*43 && local%43<40;
        assert((hit>=0)==expected);
        const int action=DownloadFolderPicker::footerHit(x,y), fx=x-20;
        assert((action>=0)==(y>=508 && y<540 && fx>=0 && fx<4*230 && fx%230<220));
    }
    picker.entries.resize(4994);assert(picker.hit(40,137+2*43)==-1); // Unused cells on the last page.
    block_open=true;picker.open("uma0:blocked","ux0:default");
    {std::unique_lock<std::mutex> lock(gate);condition.wait(lock,[]{return entered;});}
    picker.poll();picker.close();picker.open("ux0:default"); // UI remains responsive while storage is blocked.
    {std::lock_guard<std::mutex> lock(gate);block_open=false;} condition.notify_all();ready(picker);
    assert(picker.path=="ux0:default" && picker.entries.size()==4);
    picker.close();assert(!picker.active && !picker.loading);
    std::puts("Download picker: remembered/default folders, files, async cancellation, bounded lists and shared touch bounds PASS");
}
