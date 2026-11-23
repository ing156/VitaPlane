#!/usr/bin/env bash
set -euo pipefail
root=$(cd -- "$(dirname -- "$0")/.." && pwd)
build_dir=$(mktemp -d "${TMPDIR:-/tmp}/vitagram-web-proxy-tests.XXXXXX")
trap 'rm -rf -- "$build_dir"' EXIT
"${CXX:-c++}" -std=c++17 -O2 -Wall -Wextra -Werror -pthread -I "$root/src" -I "$root/build-modern/external/jansson/include" \
  "$root/src/web_proxy.cpp" "$root/src/web_proxy_protocol.cpp" "$root/src/web_proxy_http.cpp" "$root/src/update_core.cpp" \
  "$root/tests/web_proxy_transport_test.cpp" -Wl,--wrap=getaddrinfo -Wl,--wrap=pthread_create -Wl,--wrap=pthread_attr_setstacksize -lssl -lcrypto -lz -l:libjansson.so.4 -o "$build_dir/web_proxy_test"
(cd "$build_dir" && python3 "$root/tests/web_proxy_relay_test.py" "$build_dir/web_proxy_test")
