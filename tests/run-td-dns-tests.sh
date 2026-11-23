#!/usr/bin/env bash
set -euo pipefail
root=$(cd -- "$(dirname -- "$0")/.." && pwd)
work=$(mktemp -d "${TMPDIR:-/tmp}/vitagram-td-dns.XXXXXX")
trap 'rm -rf -- "$work"' EXIT
python3 - "$root" "$work" <<'PY'
from pathlib import Path
import sys
source = (Path(sys.argv[1]) / 'external/tdlib-upstream/tdnet/td/net/GetHostByNameActor.cpp').read_text()
start = source.index('GetHostByNameActor::GetHostByNameActor(Options options)')
end = source.index('\n}\n', start) + 2
constructor = source[start:end]
test = '''#include <cassert>
#include <utility>
#include <vector>
#define CHECK(value) assert(value)
struct GetHostByNameActor {
    enum class ResolverType { Native, Google };
    struct Options { std::vector<ResolverType> resolver_types{ResolverType::Native}; };
    Options options_;
    explicit GetHostByNameActor(Options options);
};
''' + constructor + '''
int main() {
    using Resolver = GetHostByNameActor::ResolverType;
    for (const auto& chain : {std::vector<Resolver>{Resolver::Native},
                              std::vector<Resolver>{Resolver::Google, Resolver::Native}}) {
        GetHostByNameActor actor(GetHostByNameActor::Options{chain});
#if TD_VITA
        assert(actor.options_.resolver_types == std::vector<Resolver>{Resolver::Google});
#else
        assert(actor.options_.resolver_types == chain);
#endif
    }
}
'''
(Path(sys.argv[2]) / 'resolver_policy.cpp').write_text(test)
PY
for vita in 0 1; do
  "${CXX:-c++}" -std=c++17 -Wall -Wextra -Werror -DTD_VITA="$vita" "$work/resolver_policy.cpp" -o "$work/resolver_policy"
  "$work/resolver_policy"
done
cat > "$work/CMakeLists.txt" <<'CMAKE'
cmake_minimum_required(VERSION 3.10)
project(vitagram_dns LANGUAGES C CXX)
set(CMAKE_CXX_STANDARD 17)
set(TD_ENABLE_TESTS OFF CACHE BOOL "" FORCE)
add_subdirectory("${VITAGRAM_ROOT}/external/tdlib-upstream" tdlib EXCLUDE_FROM_ALL)
add_executable(dns_test "${VITAGRAM_ROOT}/tests/td_dns_test.cpp"
    "${VITAGRAM_ROOT}/external/tdlib-upstream/tdutils/td/utils/port/IPAddress.cpp"
    "${VITAGRAM_ROOT}/external/tdlib-upstream/tdutils/td/utils/port/SocketFd.cpp")
target_compile_definitions(dns_test PRIVATE VITAGRAM_TD_DNS_TEST=1)
target_link_libraries(dns_test PRIVATE tdutils)
target_link_options(dns_test PRIVATE -Wl,--wrap=getaddrinfo -Wl,--wrap=freeaddrinfo -Wl,--wrap=read -Wl,--wrap=send)
CMAKE
cmake -S "$work" -B "$work/build" -DCMAKE_BUILD_TYPE=Debug -DCMAKE_POLICY_VERSION_MINIMUM=3.5 -DVITAGRAM_ROOT="$root"
cmake --build "$work/build" --target dns_test --parallel "$(nproc)"
"$work/build/dns_test"
echo "Pinned TDLib DNS/socket: numeric fast path, capped writes, ENOBUFS recovery, fatal errors and diagnostics PASS"
