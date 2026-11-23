#!/usr/bin/env bash
set -euo pipefail
root=$(cd -- "$(dirname -- "$0")/.." && pwd)
work=$(mktemp -d "${TMPDIR:-/tmp}/vitagram-vpk-tests.XXXXXX")
trap 'rm -rf -- "$work"' EXIT
"${CXX:-c++}" -std=c++17 -Wall -Wextra -Werror -fsanitize=address,undefined -g \
  -I "$root/src" "$root/src/vpk_package.cpp" "$root/tests/vpk_package_test.cpp" -lcrypto -o "$work/vpk"
"$work/vpk"

"${CXX:-c++}" -std=c++17 -O1 -g -Wall -Wextra -Werror -pthread -D__vita__ \
  -fsanitize=address,undefined -idirafter "${VITASDK:-/usr/local/vitasdk}/arm-vita-eabi/include" \
  "$root/tests/vpk_installer_test.cpp" -Wl,--wrap=pthread_create -o "$work/installer"
for scenario in fresh repeat loaded promoter-loaded paf-load-error paf-start-error promoter-error cancel decline-install thread-error progress-thread-error invalid helper promote-error; do
  mkdir -p "$work/$scenario"
  (cd "$work/$scenario" && "$work/installer" "$scenario")
done
python3 "$root/tests/vpk_progress_test.py"
