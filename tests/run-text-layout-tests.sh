#!/usr/bin/env bash
set -euo pipefail
root=$(cd -- "$(dirname -- "$0")/.." && pwd)
build_dir=$(mktemp -d "${TMPDIR:-/tmp}/vitagram-text-layout-tests.XXXXXX")
trap 'rm -rf -- "$build_dir"' EXIT
emoji_font="$root/assets/NotoColorEmoji_b-64.ttf"
if [[ ! -s "$emoji_font" ]]; then emoji_font="$root/assets/NotoColorEmoji_b.ttf"; fi
python3 "$root/scripts/build-emoji-catalog.py" "$emoji_font" "$build_dir/emoji_data.h"
"${CXX:-c++}" -std=c++17 -O2 -Wall -Wextra -Werror -I "$root/src" -I "$build_dir" "$root/tests/text_layout_test.cpp" -o "$build_dir/layout"
"$build_dir/layout"
