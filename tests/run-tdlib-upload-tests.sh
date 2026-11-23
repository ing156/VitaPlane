#!/usr/bin/env bash
set -euo pipefail
root=$(cd -- "$(dirname -- "$0")/.." && pwd)
work=$(mktemp -d "${TMPDIR:-/tmp}/vitagram-tdlib-upload.XXXXXX")
trap 'rm -rf -- "$work"' EXIT
python3 - "$root" "$work" <<'PY'
from pathlib import Path
import sys
source = (Path(sys.argv[1]) / 'external/tdlib-upstream/td/telegram/files/FileManager.cpp').read_text()
start = source.index('  if (file_view.has_full_local_location()) {', source.index('void FileManager::resume_upload('))
end = source.index('\n  if (!file_view.has_full_local_location()', start)
branch = source[start:end]
test = r'''
#include <cassert>
#include <map>
#include <optional>
#include <string>
#include <utility>
#include <iostream>
enum class FileType { Document, Photo };
struct Location { FileType file_type_ = FileType::Document; std::string path_ = "file"; int mtime_nsec_ = 1; };
struct Info { Location location_; int size_ = 10; };
struct Status { std::string reason; bool is_error() const { return !reason.empty(); } std::string message() const { return reason; } };
std::ostream& operator<<(std::ostream& out, const Status&) { return out; }
template<class T> struct Result { T value{}; bool valid = true; bool is_ok() const { return valid; } T& ok() { return value; } };
FileType checked_type = FileType::Document;
struct Node {
    struct Local { bool full_ = true; Location full() { Location value; value.file_type_ = checked_type; return value; } } local_;
    int upload_id_ = 0, download_id_ = 0, owner_dialog_id_ = 1, size = 10;
    struct Remote { bool partial = false, full = false; } remote_;
    bool generate_ = false;
    void set_size(int value) { size = value; }
} storage;
struct View { explicit View(Node* value):node(value) {} Node* node; bool has_full_local_location() { return node->local_.full_; } };
using FileView = View;
struct Upload { int get_file_id() { return 1; } };
std::ostream& operator<<(std::ostream& out, const Upload&) { return out; }
std::map<std::string, int> bad_paths_;
struct Locations { int collision = 0; int count(const Location&) { return collision != 0; } int at(const Location&) { return collision; } void erase(const Location&) { collision = 0; } } local_location_to_file_id_;
bool exists = true, registered_ok = true;
int checks = 0, registrations = 0;
std::string failure = "File size has changed";
Status check_local_location(Node* node, bool) { node->local_.full_ = false; return {failure}; }
Result<Info> check_full_local_location(Info info, bool skip) {
    assert(!skip && info.size_ == 0 && info.location_.mtime_nsec_ == 0);
    ++checks; info.size_ = 20; info.location_.mtime_nsec_ = 2;
    return {info, exists};
}
Result<int> register_local(Location location, int, int size, bool hash, bool skip, int id) {
    assert(!hash && !skip && id == 1 && location.mtime_nsec_ == 2 && size == 20);
    ++registrations; storage.local_.full_ = registered_ok;
    return {1, registered_ok};
}
Node* get_sync_file_node(int) { return &storage; }
#define LOG(level) std::cerr
void attempt() {
    Node* node = &storage; FileView file_view{node}; Upload file_upload_id;
'''+branch+r'''
}
void reset() {
    storage = Node{}; checked_type = FileType::Document; exists = registered_ok = true; checks = registrations = 0;
    bad_paths_.clear(); local_location_to_file_id_.collision = false;
    failure = "File size has changed";
}
int main() {
    reset(); attempt(); assert(storage.local_.full_ && storage.size == 20 && checks == 1 && registrations == 1);
    reset(); local_location_to_file_id_.collision = 1; attempt(); assert(storage.local_.full_ && registrations == 1);
    reset(); failure = "File was modified"; attempt(); assert(storage.local_.full_ && checks == 1);
    reset(); exists = false; attempt(); assert(!storage.local_.full_ && checks == 1 && !registrations);
    reset(); bad_paths_["file"] = 1; attempt(); assert(!storage.local_.full_ && !registrations);
    reset(); registered_ok = false; attempt(); assert(!storage.local_.full_ && registrations == 1);
    reset(); local_location_to_file_id_.collision = 2; attempt(); assert(!storage.local_.full_ && !registrations);
    reset(); failure = "Sending of internal database files is forbidden"; attempt(); assert(!checks && !registrations);
    reset(); storage.upload_id_ = 1; attempt(); assert(!checks);
    reset(); storage.download_id_ = 1; attempt(); assert(!checks);
    reset(); storage.remote_.partial = true; attempt(); assert(!checks);
    reset(); storage.remote_.full = true; attempt(); assert(!checks);
    reset(); storage.generate_ = true; attempt(); assert(!checks);
    reset(); checked_type = FileType::Photo; attempt(); assert(!checks);
}
'''
(Path(sys.argv[2]) / 'test.cpp').write_text(test)
PY
"${CXX:-c++}" -std=c++17 -O2 -Wall -Wextra -Werror -DTD_VITA=1 "$work/test.cpp" -o "$work/test"
"$work/test" 2>/dev/null
echo "TDLib upload recheck: changed document, missing/forbidden file, failed registration and in-flight guards PASS"
