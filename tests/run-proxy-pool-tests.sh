#!/usr/bin/env bash
set -euo pipefail
root=$(cd -- "$(dirname -- "$0")/.." && pwd)
build_dir=$(mktemp -d "${TMPDIR:-/tmp}/vitagram-proxy-pool-tests.XXXXXX")
trap 'rm -rf -- "$build_dir"' EXIT
"${CXX:-c++}" -std=c++17 -O2 -Wall -Wextra -Werror -I "$root/src" \
  "$root/tests/proxy_pool_test.cpp" -o "$build_dir/pool"
"$build_dir/pool"
"${CXX:-c++}" -std=c++17 -O2 -Wall -Wextra -Werror -pthread -I "$root/src" -I "$root/build-modern/external/jansson/include" \
  "$root/tests/proxy_settings_test.cpp" "$root/src/vault.cpp" "$root/src/vault_crypto.cpp" "$root/src/secure_store.cpp" \
  -Wl,--wrap=fopen -Wl,--wrap=rename -l:libjansson.so.4 -lcrypto -o "$build_dir/settings"
(cd "$build_dir" && ./settings)
"${CXX:-c++}" -std=c++17 -O2 -Wall -Wextra -Werror -I "$root/src" -I "$root/build-modern/external/jansson/include" \
  "$root/tests/proxy_settings_crypto_test.cpp" -l:libjansson.so.4 -o "$build_dir/crypto_failure"
(cd "$build_dir" && ./crypto_failure)
