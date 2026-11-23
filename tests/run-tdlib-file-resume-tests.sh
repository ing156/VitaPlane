#!/usr/bin/env bash
# Host-only pinned TDLib codec/SQLite persistence check; no FileManager actors or Telegram network.
set -euo pipefail
root=$(cd -- "$(dirname -- "$0")/.." && pwd)
for command in cmake "${CXX:-c++}" "${CC:-cc}"; do
  command -v "$command" >/dev/null || { echo "Missing host build prerequisite: $command" >&2; exit 1; }
done
for header in telegram_api.h td_api.h; do
  [[ -s "$root/external/tdlib-upstream/td/generate/auto/td/telegram/$header" ]] || {
    echo "Missing pinned TDLib generated header: $header; prepare TDLib sources as in docs/building.md" >&2
    exit 1
  }
done
work=$(mktemp -d "${TMPDIR:-/tmp}/vitagram-tdlib-file-resume.XXXXXX")
trap 'rm -rf -- "$work"' EXIT
cat > "$work/CMakeLists.txt" <<'CMAKE'
cmake_minimum_required(VERSION 3.10)
project(vitagram_file_resume LANGUAGES C CXX)
set(CMAKE_CXX_STANDARD 17)
set(CMAKE_CXX_STANDARD_REQUIRED ON)
find_package(OpenSSL REQUIRED)
set(TD_ENABLE_TESTS OFF CACHE BOOL "" FORCE)
set(TD_ENABLE_JNI OFF CACHE BOOL "" FORCE)
set(TD_ENABLE_DOTNET OFF CACHE BOOL "" FORCE)
add_subdirectory("${VITAGRAM_ROOT}/external/tdlib-upstream" tdlib EXCLUDE_FROM_ALL)
add_executable(file_resume "${VITAGRAM_ROOT}/tests/tdlib_file_resume_test.cpp"
    "${VITAGRAM_ROOT}/external/tdlib-upstream/td/telegram/files/FileBitmask.cpp")
target_include_directories(file_resume PRIVATE "${VITAGRAM_ROOT}/external/tdlib-upstream"
    "${VITAGRAM_ROOT}/external/tdlib-upstream/td/generate/auto")
target_link_libraries(file_resume PRIVATE tddb)
CMAKE
cmake -S "$work" -B "$work/build" -DCMAKE_BUILD_TYPE=Debug -DCMAKE_POLICY_VERSION_MINIMUM=3.5 -DVITAGRAM_ROOT="$root"
cmake --build "$work/build" --target file_resume --parallel "$(nproc)"
"$work/build/file_resume" write "$work/files.sqlite" "$work/partial.bin"
"$work/build/file_resume" read "$work/files.sqlite" "$work/partial.bin"
