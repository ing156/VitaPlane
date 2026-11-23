#!/usr/bin/env bash
set -euo pipefail
root=$(cd -- "$(dirname -- "$0")/.." && pwd)
work=$(mktemp -d "${TMPDIR:-/tmp}/vitagram-sqlite-vita.XXXXXX")
trap 'rm -rf -- "$work"' EXIT
"${CC:-cc}" -std=gnu99 -O1 -D_GNU_SOURCE -include unistd.h -include errno.h -DSQLITE_MAX_MMAP_SIZE=0 -DNDEBUG -DVITAGRAM_SQLITE_VITA_TEST -DSQLITE_HAS_CODEC -DOMIT_MEMLOCK -DSQLITE_OMIT_WAL -DSQLITE_OMIT_LOAD_EXTENSION \
  -c "$root/external/tdlib-upstream/sqlite/sqlite/sqlite3.c" -o "$work/sqlite.o"
"${CC:-cc}" -std=gnu99 -O1 -I "$root/external/tdlib-upstream/sqlite/sqlite" \
  "$root/tests/sqlite_vita_test.c" "$work/sqlite.o" -Wl,--wrap=fcntl -lpthread -lm -lcrypto -lz -o "$work/test"
(cd "$work" && ./test)
