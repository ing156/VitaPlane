#!/usr/bin/env bash
set -euo pipefail
root=$(cd -- "$(dirname -- "$0")/.." && pwd)
work=$(mktemp -d "${TMPDIR:-/tmp}/vitagram-disabled-db.XXXXXX")
trap 'rm -rf -- "$work"' EXIT
python3 - "$root" "$work" <<'PY'
from pathlib import Path
import re,sys
source=(Path(sys.argv[1])/'external/tdlib-upstream/td/telegram/TdDb.cpp').read_text()
def block(pattern):
    matches=re.findall(pattern,source,re.S)
    assert len(matches)==1
    return matches[0]
disabled=block(r'  if \(!use_sqlite\) \{.*?\n  \}')
key=block(r'  if \(drop_sqlite_key\) \{.*?\n  \}')
text='''#include <cassert>
#include <cstdio>
#include <fstream>
#include <iterator>
#include <map>
#include <string>
struct Status { static Status OK() { return {}; } void ignore() {} };
struct SqliteDb { static Status destroy(const std::string& path) { std::remove(path.c_str());return {}; } };
struct Parameters { bool use_file_database_=false; };
struct Auto {};
struct Binlog : std::map<std::string,std::string> { int syncs=0;void force_sync(Auto,const char*) { ++syncs; } };
Status init_disabled(bool use_sqlite,const std::string& sql_database_path) {
'''+disabled+'''
return Status::OK();
}
void drop_key(bool drop_sqlite_key,Parameters parameters,Binlog* binlog_pmc) {
'''+key+'''
}
int main() {
  const std::string path="file-index.sqlite",payload="public-index-fixture";
  { std::ofstream file(path);file<<payload; }
  init_disabled(false,path);
  std::ifstream file(path);
#if TD_VITA
  assert(file);
  const std::string data{std::istreambuf_iterator<char>(file),std::istreambuf_iterator<char>()};assert(data==payload);
#else
  assert(!file);
#endif
  Binlog keys;keys["sqlite_key"]="public-key-fixture";
  drop_key(true,Parameters{false},&keys);
#if TD_VITA
  assert(keys.at("sqlite_key")=="public-key-fixture" && keys.syncs==0);
#else
  assert(keys.empty());
#endif
  keys["sqlite_key"]="public-key-fixture";
  drop_key(true,Parameters{true},&keys);assert(keys.empty());
}
'''
(Path(sys.argv[2])/'test.cpp').write_text(text)
PY
for vita in 1 0; do
  "${CXX:-c++}" -std=c++17 -O2 -DTD_VITA="$vita" "$work/test.cpp" -o "$work/test"
  (cd "$work" && ./test)
done
echo "Pinned TDLib disabled-file-database branch: Vita index/key preservation and normal key migration PASS"
