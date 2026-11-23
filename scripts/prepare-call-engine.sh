#!/usr/bin/env bash
# Build the pinned WebRTC and tgcalls libraries for the optional Vita call build.
set -euo pipefail
: "${VITASDK:?Set VITASDK to your VitaSDK directory}"
export PATH="$VITASDK/bin:$PATH"
project=$(cd -- "$(dirname -- "${BASH_SOURCE[0]}")/.." && pwd)
work="$project/build-modern/call-port"
mkdir -p "$work"

checkout() {
    local name=$1 url=$2 revision=$3 destination="$work/$1"
    if [[ ! -e "$destination/.git" ]]; then
        git init -q "$destination"
        git -C "$destination" fetch --depth 1 "$url" "$revision"
        git -C "$destination" checkout --detach FETCH_HEAD
    fi
    [[ $(git -C "$destination" rev-parse HEAD) == "$revision" ]] || {
        printf 'Unexpected revision in %s\n' "$destination" >&2
        exit 1
    }
}
apply_patch() {
    local name=$1 patch="$project/scripts/$1-vita.patch"
    if ! git -C "$work/$name" apply --reverse --check "$patch" 2>/dev/null; then
        git -C "$work/$name" apply "$patch"
    fi
}

checkout tg_owt https://github.com/desktop-app/tg_owt.git 642ddb107b42da7c8d152401b71cd429475e5266
checkout tgcalls https://github.com/TelegramMessenger/tgcalls.git f0bffbb7ba91a4b95136f9bd9354d3eae9226b7a
checkout openh264 https://github.com/cisco/openh264.git 652bdb7719f30b52b08e506645a7322ff1b2cc6f
checkout libvpx https://github.com/webmproject/libvpx.git d168454ecd099805c675d4a98c66f4891373302a
if git -C "$work/tg_owt" submodule status | rg -q '^-'; then
    git -C "$work/tg_owt" -c submodule.src/third_party/libyuv.url=https://github.com/lemenkov/libyuv.git \
        submodule update --init --depth 1
fi
apply_patch tg_owt
apply_patch tgcalls
apply_patch openh264

if [[ ! -f "$work/install/lib/libopenh264.a" ]]; then
    make -C "$work/openh264" -j"$(nproc)" OS=linux ARCH=arm CC=arm-vita-eabi-gcc CXX=arm-vita-eabi-g++ \
        AR=arm-vita-eabi-ar ASM=arm-vita-eabi-gcc PREFIX="$work/install" \
        CFLAGS_OPT='-O2 -marm -mfpu=neon -mfloat-abi=hard' install-static
fi
if [[ ! -f "$work/install/lib/libvpx.a" ]]; then
    mkdir -p "$work/build-libvpx"
    (
        cd "$work/build-libvpx"
        CROSS=arm-vita-eabi- "$work/libvpx/configure" --target=armv7-linux-gcc --prefix="$work/install" \
            --disable-examples --disable-tools --disable-docs --disable-unit-tests --disable-multithread \
            --disable-runtime-cpu-detect --disable-pic --extra-cflags='-marm -mfpu=neon -mfloat-abi=hard'
        make -j"$(nproc)" install
    )
fi

if [[ ! -f "$work/openssl-dtls/lib/libssl.a" ]]; then
    archive="$project/external/openssl-vita/openssl-3.5.9.tar.gz"
    if [[ ! -f "$archive" ]]; then
        archive="$work/openssl-3.5.9.tar.gz"
        curl --fail --location --retry 3 \
            https://github.com/openssl/openssl/releases/download/openssl-3.5.9/openssl-3.5.9.tar.gz -o "$archive"
    fi
    printf '%s  %s\n' 603f5602e2eef00d77fbd429d34dcd5822bb301757a1bc9cdb24c670f1eb859a "$archive" | sha256sum --check --quiet
    if [[ ! -d "$work/openssl-3.5.9" ]]; then
        tar -xzf "$archive" -C "$work"
        patch --directory="$work/openssl-3.5.9" --strip=1 --forward --input="$project/scripts/openssl-vita.patch"
    fi
    mkdir -p "$work/build-openssl"
    (
        cd "$work/build-openssl"
        "$work/openssl-3.5.9/Configure" vita-cross --config="$project/scripts/openssl-vita.conf" \
            --prefix="$work/openssl-dtls" --libdir=lib --openssldir=/nonexistent --with-rand-seed=getrandom \
            no-shared no-asm no-dso no-module no-engine no-apps no-tests no-docs no-atexit \
            no-async no-afalgeng no-ktls no-quic no-sctp no-ssl3 no-comp no-ec2m no-legacy no-cms \
            no-ts no-cmp no-srp no-http no-ui-console no-secure-memory no-thread-pool no-default-thread-pool
        make -j"$(nproc)" build_libs
        make install_dev
    )
    install -Dm644 "$work/openssl-3.5.9/LICENSE.txt" "$work/openssl-dtls/share/OpenSSL-LICENSE.txt"
fi

ffmpeg="${VITAGRAM_FFMPEG_ROOT:-$project/external/ffmpeg-vita71/install}"
export PKG_CONFIG_LIBDIR="$work/install/lib/pkgconfig:$ffmpeg/lib/pkgconfig:$work/openssl-dtls/lib/pkgconfig:$VITASDK/arm-vita-eabi/lib/pkgconfig"
cmake -S "$work/tg_owt" -B "$work/build-webrtc" -DCMAKE_TOOLCHAIN_FILE="$VITASDK/share/vita.toolchain.cmake" \
    -DCMAKE_BUILD_TYPE=Release -DCMAKE_CXX_FLAGS_RELEASE='-O1 -DNDEBUG -Wno-psabi' \
    -DCMAKE_INSTALL_PREFIX="$work/install" -DTG_OWT_USE_X11=OFF -DTG_OWT_USE_PIPEWIRE=OFF \
    -DPKG_CONFIG_EXECUTABLE="$(command -v pkg-config)" -DOPENSSL_INCLUDE_DIR="$work/openssl-dtls/include" \
    -DOPENSSL_SSL_LIBRARY="$work/openssl-dtls/lib/libssl.a" -DOPENSSL_CRYPTO_LIBRARY="$work/openssl-dtls/lib/libcrypto.a" \
    -DTG_OWT_VITA_PROBE="$project/scripts/vita-call-engine.cmake"
cmake --build "$work/build-webrtc" --target tgcalls_vita_probe --parallel "$(nproc)"
