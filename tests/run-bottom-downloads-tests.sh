#!/usr/bin/env bash
set -euo pipefail
root=$(cd -- "$(dirname -- "$0")/.." && pwd)
work=$(mktemp -d "${TMPDIR:-/tmp}/vitagram-bottom-downloads.XXXXXX")
trap 'rm -rf -- "$work"' EXIT
"${CXX:-c++}" -std=c++17 -Wall -Wextra -Werror -fsanitize=address,undefined -fno-omit-frame-pointer -I "$root/src" \
  "$root/tests/bottom_downloads_test.cpp" -o "$work/test"
"$work/test"
python3 "$root/tests/bottom_downloads_controller_test.py"
