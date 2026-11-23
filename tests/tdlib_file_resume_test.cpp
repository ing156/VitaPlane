// Exercises the pinned partial-location codec and SQLite KV store across separate processes.
// This deliberately excludes FileManager/FileDb actors and live Telegram transfer scheduling.
#include "td/telegram/files/FileLocation.hpp"
#include "td/telegram/files/FileBitmask.h"
#include "td/db/DbKey.h"
#include "td/db/SqliteDb.h"
#include "td/db/SqliteKeyValue.h"
#include "td/utils/tl_helpers.h"
#include <cassert>
#include <cstdio>
#include <fstream>
#include <string>

int main(int argc, char** argv) {
    assert(argc == 4);
    const std::string mode=argv[1], database=argv[2], path=argv[3];
    constexpr td::int64 part_size=65536, file_size=part_size*6+123;
    auto opened=td::SqliteDb::open_with_key(database,mode=="write",td::DbKey::empty());
    opened.ensure();
    auto db=opened.move_as_ok();
    td::SqliteKeyValue::init(db,"files").ensure();
    td::SqliteKeyValue kv;
    kv.init_with_connection(std::move(db),"files").ensure();
    if(mode=="write") {
        td::Bitmask ready;
        for(auto index:{0,2,5,6}) ready.set(index);
        td::PartialLocalFileLocation location{td::FileType::Document,part_size,path,"",ready.encode(),0};
        // A sparse file's length includes holes; only the stored mask identifies completed parts.
        std::ofstream file(path,std::ios::binary);
        for(auto index:{0,2,5,6}) {
            file.seekp(index*part_size);
            std::string bytes(index==6 ? 123 : part_size,char('A'+index));
            file.write(bytes.data(),bytes.size());
        }
        file.close(); assert(file);
        kv.begin_write_transaction().ensure();
        kv.set("file1",td::serialize(location));
        kv.commit_transaction().ensure();
    } else {
        assert(mode=="read");
        const auto serialized=kv.get("file1"); assert(!serialized.empty());
        td::PartialLocalFileLocation restored{};
        td::unserialize(restored,serialized).ensure();
        assert(restored.file_type_==td::FileType::Document && restored.path_==path);
        assert(restored.part_size_==part_size && restored.iv_.empty());
        td::Bitmask ready(td::Bitmask::Decode{},restored.ready_bitmask_);
        for(td::int64 i=0;i<7;++i) assert(ready.get(i)==(i==0 || i==2 || i==5 || i==6));
        assert(ready.get_total_size(part_size,file_size)==part_size*3+123);
        assert(ready.get_ready_prefix_size(0,part_size,file_size)==part_size);
        assert(ready.get_ready_prefix_size(part_size,part_size,file_size)==0);
        assert(ready.get_ready_prefix_size(part_size*2,part_size,file_size)==part_size);
        assert(ready.get_ready_prefix_size(part_size*5,part_size,file_size)==part_size+123);
        std::ifstream file(restored.path_,std::ios::binary); assert(file);
        file.seekg(0,std::ios::end); assert(file.tellg()==file_size);
        for(auto index:{0,1,2,3,4,5,6}) {
            file.seekg(index*part_size); char byte=0; file.get(byte); assert(file);
            assert(byte==((index==0 || index==2 || index==5 || index==6) ? char('A'+index) : '\0'));
        }
        std::puts("Pinned TDLib partial-location codec + SQLite reopen: sparse ready mask/path/part size/prefix and holes: PASS");
    }
    kv.close();
}
