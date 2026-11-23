#!/usr/bin/env bash
set -euo pipefail
root=$(cd -- "$(dirname -- "$0")/.." && pwd)
build_dir=$(mktemp -d "${TMPDIR:-/tmp}/vitagram-font-tests.XXXXXX")
trap 'rm -rf -- "$build_dir"' EXIT
emoji_font="$root/assets/NotoColorEmoji_b-64.ttf"
if [[ ! -s "$emoji_font" ]]; then emoji_font="$root/assets/NotoColorEmoji_b.ttf"; fi
python3 "$root/scripts/build-emoji-catalog.py" "$emoji_font" "$build_dir/emoji_data.h"
headers=${FREETYPE_INCLUDE_DIR:-/usr/include/freetype2}
if [[ ! -f "$headers/ft2build.h" ]]; then headers="${VITASDK:-/usr/local/vitasdk}/arm-vita-eabi/include/freetype2"; fi
"${CXX:-c++}" -std=c++17 -Wall -Wextra -Werror -I "$root/src" -I "$build_dir" -I "$headers" \
  "$root/tests/font_fallback_test.cpp" "$root/src/utf8_decode.cpp" -l:libfreetype.so.6 -o "$build_dir/fonts"
"$build_dir/fonts" "$root/assets"

"${CXX:-c++}" -std=c++17 -Wall -Wextra -Werror -I "$root/tests/font-stubs" -I "$root/src" -I "$build_dir" -I "$headers" \
  "$root/tests/text_font_test.cpp" "$root/src/text_font.cpp" "$root/src/utf8_decode.cpp" \
  -Wl,--wrap=FT_Render_Glyph -Wl,--wrap=FT_Set_Pixel_Sizes -Wl,--wrap=FT_New_Size -Wl,--wrap=FT_Done_Size \
  -l:libfreetype.so.6 -o "$build_dir/text-font"
"$build_dir/text-font" "$root/assets"

"${CXX:-c++}" -std=c++17 -Wall -Wextra -Werror -pthread -DVITAGRAM_PERFORMANCE=0 \
  -I "$root/tests/font-stubs" -I "$root/src" -I "$build_dir" -I "$headers" \
  "$root/tests/language_font_test.cpp" "$root/src/text_font.cpp" "$root/src/utf8_decode.cpp" \
  -l:libfreetype.so.6 -l:libwebp.so.7 -o "$build_dir/language-font"
"$build_dir/language-font" "$root/assets"

"${CXX:-c++}" -std=c++17 -Wall -Wextra -Werror -I "$root/src" -I "$build_dir" -I "$headers" \
  "$root/tests/emoji_test.cpp" -l:libfreetype.so.6 -o "$build_dir/emoji"
"$build_dir/emoji" "$emoji_font"
python3 "$root/tests/emoji_layout_test.py" "$build_dir"
