#!/usr/bin/env bash
set -euo pipefail
root=$(cd -- "$(dirname -- "$0")/.." && pwd)
build_dir=$(mktemp -d "${TMPDIR:-/tmp}/vitagram-image-tests.XXXXXX")
trap 'rm -rf -- "$build_dir"' EXIT
tgs_flags=() tgs_libs=()
if [[ -n "${RLOTTIE_NATIVE_BUILD:-}" ]]; then
  tgs_flags=(-DVITAGRAM_RLOTTIE=1 -I "$root/external/rlottie-vita/source/inc" "-DVITAGRAM_TEST_TGS_JSON=\"$root/tests/fixtures/tgs-preview.json\"")
  tgs_libs=("$RLOTTIE_NATIVE_BUILD/librlottie.a" -lz)
fi
"${CXX:-c++}" -std=c++17 -O2 -Wall -Wextra -Werror -pthread \
  "${tgs_flags[@]}" -I "$root/tests/stubs" -I "$root/src" \
  "$root/src/image_decoder.cpp" "$root/src/vault.cpp" "$root/src/vault_crypto.cpp" "$root/src/secure_store.cpp" "$root/tests/image_decoder_test.cpp" -l:libjansson.so.4 -I "$root/build-modern/external/jansson/include" -lcrypto \
  $(pkg-config --cflags --libs libpng libjpeg libwebp) "${tgs_libs[@]}" -o "$build_dir/images"
(cd "$build_dir" && ./images)
