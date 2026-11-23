#!/usr/bin/env bash
set -euo pipefail
root=$(cd -- "$(dirname -- "$0")/.." && pwd)
build_dir=$(mktemp -d "${TMPDIR:-/tmp}/vitagram-proxy-diagnostics.XXXXXX")
trap 'rm -rf -- "$build_dir"' EXIT
"${CXX:-c++}" -std=c++17 -O2 -Wall -Wextra -Werror -pthread -DVITAGRAM_VERSION='"diagnostics-test"' \
  -I "$root/src" "$root/tests/proxy_diagnostics_test.cpp" \
  -Wl,--wrap=fopen -Wl,--wrap=rename -Wl,--wrap=stat -o "$build_dir/proxy_diagnostics"
(cd "$build_dir" && ./proxy_diagnostics)
